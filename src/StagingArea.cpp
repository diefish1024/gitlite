#include "../include/StagingArea.h"

#include "../include/Utils.h"
#include "../include/Blob.h"
#include <string>
#include <fstream>

void StagingArea::addFile(const std::string& filepath) {
    Blob blob = Blob::fromFile(filepath);
    blob.save();
    staged_files_[filepath] = blob.getSHA1();
    removed_files_.erase(filepath);
}

void StagingArea::unstage(const std::string& filepath) {
    staged_files_.erase(filepath);
}

void StagingArea::stageForRemoval(const std::string& filepath) {
    staged_files_.erase(filepath);
    removed_files_.insert(filepath);
}

std::map<std::string, std::string> StagingArea::getStagedFiles() const {
    return staged_files_;
}

std::set<std::string> StagingArea::getRemovedFiles() const {
    return removed_files_;
}

bool StagingArea::isEmpty() const {
    return staged_files_.empty() && removed_files_.empty();
}

void StagingArea::clear() {
    staged_files_.clear();
    removed_files_.clear();
}

void StagingArea::printStagedFiles() const {
    for (const auto& pair : staged_files_) {
        std::cout << pair.first << std::endl;
    }
}

void StagingArea::printRemovedFiles() const {
    for (const auto& file : removed_files_) {
        std::cout << file << std::endl;
    }
}

void StagingArea::save(const std::string& filepath) const {
    std::ofstream ofs(filepath);
    for (const auto& pair : staged_files_) {
        ofs << "+ " << pair.first << " " << pair.second << "\n";
    }
    for (const auto& path : removed_files_) {
        ofs << "- " << path << "\n";
    }
    ofs.close();
}

void StagingArea::load(const std::string& filepath) {
    staged_files_.clear();
    removed_files_.clear();
    
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) return;

    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string type, path, sha1;
        
        iss >> type;
        if (type == "+") {
            iss >> path >> sha1;
            staged_files_[path] = sha1;
        } else if (type == "-") {
            iss >> path;
            removed_files_.insert(path);
        }
    }
    ifs.close();
}