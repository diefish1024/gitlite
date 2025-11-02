#include "../include/Commit.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include "../include/Utils.h"
#include "../include/GitliteException.h"

std::string Commit::Seralize() const {
    std::ostringstream oss;
    oss << "timestamp:" << timestamp_ << "\n";
    oss << "message:" << message_ << "\n";
    oss << "parents:";
    for (const auto& parent : parent_commits_) {
        oss << parent << " ";
    }
    oss << "\n";
    oss << "files:\n";
    for (const auto& pr : file_blob_) {
        oss << pr.first << ":" << pr.second << "\n";
    }
    return oss.str();
}

Commit Commit::create(const std::string &message,
    const std::map<std::string, std::string> &files,
    const std::string& parent,
    const time_t timestamp
) {
    Commit res(files,
        parent.empty() ? std::vector<std::string>() : std::vector<std::string>{parent},
        message,
        timestamp,
        "");
    
    std::string content = res.Seralize();
    std::string header = "commit " + std::to_string(content.length()) + '\0';
    res.sha1_ = Utils::sha1(header + content);
    return res;
}

Commit Commit::create(const std::string &message,
    const std::map<std::string, std::string> &files,
    const std::vector<std::string>& parents,
    const time_t timestamp
) {
    Commit res(files,
        parents,
        message,
        timestamp,
        "");
    std::string content = res.Seralize();
    std::string header = "commit " + std::to_string(content.length()) + '\0';
    res.sha1_ = Utils::sha1(header + content);
    return res;
}

Commit Commit::load(const std::string &filepath) {
    if (!Utils::exists(filepath)) {
        throw GitliteException("Commit not found: " + filepath);
    }

    std::string full_content = Utils::readContentsAsString(filepath);
    size_t null_pos = full_content.find('\0');
    if (null_pos == std::string::npos) {
        throw GitliteException("Invalid commit object: no header found in " + filepath);
    }

    std::string header = full_content.substr(0, null_pos);
    std::string payload = full_content.substr(null_pos + 1);

    std::istringstream hss(header);
    std::string type;
    size_t size;
    hss >> type >> size;

    if (type != "commit" || size != payload.length()) {
        throw GitliteException("Invalid commit object: malformed header in " + filepath);
    }
    
    std::istringstream iss(payload);
    std::string line;
    std::map<std::string, std::string> file_blob;
    std::vector<std::string> parents;
    std::string message;
    time_t timestamp = 0;
    while (std::getline(iss, line)) {
        if (line.rfind("timestamp:", 0) == 0) {
            timestamp = std::stol(line.substr(10));
        } else if (line.rfind("message:", 0) == 0) {
            message = line.substr(8);
        } else if (line.rfind("parents:", 0) == 0) {
            std::string parents_line = line.substr(8);
            std::istringstream pss(parents_line);
            std::string parent_sha1;
            while (pss >> parent_sha1) {
                parents.push_back(parent_sha1);
            }
        } else if (line.rfind("files:", 0) == 0) {
            while (std::getline(iss, line) && !line.empty()) {
                size_t delim_pos = line.find(':');
                if (delim_pos != std::string::npos) {
                    std::string filename = line.substr(0, delim_pos);
                    std::string blob_sha1 = line.substr(delim_pos + 1);
                    file_blob[filename] = blob_sha1;
                }
            }
        }
    }

    return create(message, file_blob, parents, timestamp);
}

void Commit::save() {
    std::string dir = Utils::join(".gitlite", "objects", sha1_.substr(0, 2));
    std::string filepath = Utils::join(dir, sha1_.substr(2));

    if (!Utils::exists(dir)) {
        Utils::createDirectories(dir);
    }

    std::string content = Seralize();
    std::string header = "commit " + std::to_string(content.length()) + '\0';
    Utils::writeContents(filepath, header + content);
}

void Commit::addFile(const std::string &fileName, const std::string &blob_sha) {
    file_blob_[fileName] = blob_sha;
}