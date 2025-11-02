#include "../include/Blob.h"

#include <iostream>
#include <string>
#include <sstream>
#include "../include/Utils.h"
#include "../include/GitliteException.h"

Blob Blob::fromContent(const std::string &content) {
    std::string header = "blob " + std::to_string(content.length()) + '\0';
    std::string sha1 = Utils::sha1(header + content);
    return Blob(content, sha1);
}

Blob Blob::fromFile(const std::string& filepath) {
    std::string content = Utils::readContentsAsString(filepath);
    std::string header = "blob " + std::to_string(content.length()) + '\0';
    std::string sha1 = Utils::sha1(header + content);
    return Blob(content, sha1);
}

Blob Blob::load(const std::string& sha1) {
    std::string dir = Utils::join(".gitlite", "objects", sha1.substr(0, 2));
    std::string filepath = Utils::join(dir, sha1.substr(2));

    if (!Utils::exists(filepath)) {
        Utils::exitWithMessage("Blob object does not exist.");
    }

    std::string full_content = Utils::readContentsAsString(filepath);
    size_t null_pos = full_content.find('\0');
    if (null_pos == std::string::npos) {
        Utils::exitWithMessage("Invalid blob object: no header found.");
    }

    std::string header = full_content.substr(0, null_pos);
    std::string content = full_content.substr(null_pos + 1);

    std::istringstream hss(header);
    std::string type;
    size_t size;
    hss >> type >> size;
    if (type != "blob" || size != content.length()) {
        Utils::exitWithMessage("Invalid blob object: malformed header.");
    }

    return Blob(content, sha1);
}

void Blob::save() {
    std::string dir = Utils::join(".gitlite", "objects", sha1_.substr(0, 2));
    std::string filepath = Utils::join(dir, sha1_.substr(2));

    if (Utils::exists(filepath)) {
        return;
    }

    Utils::createDirectories(dir);

    std::string header = "blob " + std::to_string(content_.length()) + '\0';
    Utils::writeContents(filepath, header + content_);
    return;
}