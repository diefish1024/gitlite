#include "../include/StagingArea.h"

#include "../include/Utils.h"
#include "../include/Blob.h"
#include <string>
#include <fstream>

void StagingArea::addFile(const std::string& filepath) {
    Blob blob = Blob::fromFile(filepath);
    if (blob.save()) {
        staged_files_[filepath] = blob.getSHA1();
    }
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

bool StagingArea::isEmpty() const {
    return staged_files_.empty();
}

void StagingArea::clear() {
    staged_files_.clear();
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

void StagingArea::save(std::string filepath) const {
    std::ofstream ofs(filepath);
    for (const auto& pair : staged_files_) {
        ofs << pair.first << " " << pair.second << std::endl;
    }
    ofs.close();
}

void StagingArea::load(std::string filepath) {
    staged_files_.clear();
    std::ifstream ifs(filepath);
    std::string file_path, sha1;
    while (ifs >> file_path >> sha1) {
        staged_files_[file_path] = sha1;
    }
    ifs.close();
}