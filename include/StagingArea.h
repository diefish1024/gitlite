#pragma once

#include <iostream>
#include <string>
#include <map>
#include <set>

class StagingArea {
private:
    std::map<std::string, std::string> staged_files_; // file path -> blob sha1
    std::set<std::string> removed_files_;

public:
    void printStagedFiles() const;
    void printRemovedFiles() const;

    void save(const std::string& filepath) const;
    void load(const std::string& filepath);

    void addFile(const std::string& filepath);
    void unstage(const std::string& filepath);
    void stageForRemoval(const std::string& filepath);

    std::map<std::string, std::string> getStagedFiles() const;
    std::set<std::string> getRemovedFiles() const;

    bool isEmpty() const;
    void clear();
};