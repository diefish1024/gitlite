#include "../include/Repository.h"

#include <bits/types/time_t.h>
#include "../include/Utils.h"

const std::string Repository::GITLITE_DIR_PATH = ".gitlite";
const std::string Repository::OBJECTS_DIR_PATH = Utils::join(Repository::GITLITE_DIR_PATH, "objects");
const std::string Repository::REFS_DIR_PATH = Utils::join(Repository::GITLITE_DIR_PATH, "refs");
const std::string Repository::HEADS_DIR_PATH = Utils::join(Repository::REFS_DIR_PATH, "heads");
const std::string Repository::INDEX_FILE_PATH = Utils::join(Repository::GITLITE_DIR_PATH, "index");

const std::string& Repository::getGitliteDir() {
    return GITLITE_DIR_PATH;
}

void Repository::loadCommits() {
    std::vector<std::string> object_paths = Utils::listObjectFiles(OBJECTS_DIR_PATH);

    for (const auto& object_path : object_paths) {
        std::ifstream file(object_path, std::ios::binary);
        if (!file.is_open()) {
            continue;
        }

        std::string header_part;
        std::getline(file, header_part, '\0');
        file.close();

        std::string object_type;
        std::istringstream hss(header_part);
        hss >> object_type;

        if (object_type == "commit") {
            // TODO: 异常处理
            Commit c = Commit::load(object_path);
            // commits_[c.getSHA1()] = c;
            commits_.emplace(c.getSHA1(), c);
        }
    }
}

void Repository::init() {
    // TODO: 异常抛出
    if (!Utils::exists(GITLITE_DIR_PATH)) {
        Utils::createDirectories(GITLITE_DIR_PATH);
    }
    if (!Utils::exists(OBJECTS_DIR_PATH)) {
        Utils::createDirectories(OBJECTS_DIR_PATH);
    }
    if (!Utils::exists(REFS_DIR_PATH)) {
        Utils::createDirectories(REFS_DIR_PATH);
    }
    if (!Utils::exists(HEADS_DIR_PATH)) {
        Utils::createDirectories(HEADS_DIR_PATH);
    }
}

void Repository::add(const std::string& filepath) {
    if (!Utils::exists(filepath) || !Utils::isFile(filepath)) {
        Utils::exitWithMessage("File does not exist.");
    }
    staging_area_.addFile(filepath);
    staging_area_.save(INDEX_FILE_PATH);
}

void Repository::commit(const std::string& message) {
    if (message.empty()) {
        Utils::exitWithMessage("Please enter a commit message.");
    }

    if (staging_area_.getStagedFiles().empty()) {
        Utils::exitWithMessage("No changes added to the commit.");
    }

    Commit new_commit = Commit::create(message, staging_area_.getStagedFiles(), head_);
    new_commit.save();

    head_ = new_commit.getSHA1();

    staging_area_.clear();
    staging_area_.save(INDEX_FILE_PATH);
}

void Repository::rm(const std::string& filepath) {
    const auto& staged_files = staging_area_.getStagedFiles();
    bool is_staged = staged_files.count(filepath) > 0;
    bool is_tracked = false;

    if (!head_.empty() && commits_.count(head_)) {
        const auto& tracked_files = commits_.at(head_).getTrackedFiles();
        if (tracked_files.count(filepath)) {
            is_tracked = true;
        }
    }

    if (!is_staged && !is_tracked) {
        Utils::exitWithMessage("No reason to remove the file.");
    }

    if (is_staged) {
        staging_area_.unstage(filepath);
    }

    if (is_tracked) {
        staging_area_.stageForRemoval(filepath);
        
        if (Utils::exists(filepath)) {
            Utils::restrictedDelete(filepath);
        }
    }

    staging_area_.save(INDEX_FILE_PATH);
}

void Repository::status() const {
    std::cout << "=== Branches ===" << std::endl;
    std::cout << "*master" << std::endl; // TODO: 当前分支
    std::cout << std::endl;
    std::cout << "=== Staged Files ===" << std::endl;
    staging_area_.printStagedFiles();
    std::cout << std::endl;
    std::cout << "=== Removed Files ===" << std::endl;
    staging_area_.printRemovedFiles();
    std::cout << std::endl;
    std::cout << "=== Modifications Not Staged For Commit ===" << std::endl;
    // TODO
    std::cout << std::endl;
    std::cout << "=== Untracked Files ===" << std::endl;
    // TODO
    std::cout << std::endl;
}