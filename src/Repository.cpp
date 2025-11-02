#include "../include/Repository.h"

#include <bits/types/time_t.h>
#include "../include/Blob.h"
#include "../include/Utils.h"

const std::string Repository::GITLITE_DIR_PATH = ".gitlite";
const std::string Repository::OBJECTS_DIR_PATH = Utils::join(Repository::GITLITE_DIR_PATH, "objects");
const std::string Repository::REFS_DIR_PATH = Utils::join(Repository::GITLITE_DIR_PATH, "refs");
const std::string Repository::HEADS_DIR_PATH = Utils::join(Repository::REFS_DIR_PATH, "heads");
const std::string Repository::INDEX_FILE_PATH = Utils::join(Repository::GITLITE_DIR_PATH, "index");
const std::string Repository::HEAD_FILE_PATH = Utils::join(Repository::GITLITE_DIR_PATH, "HEAD");

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
            commits_.emplace(c.getSHA1(), c);
        }
    }
}

void Repository::loadStagingArea() {
    staging_area_.load(INDEX_FILE_PATH);
}

void Repository::loadHead() {
    head_ = "";
    cur_branch_ = "master";

    if (!Utils::exists(HEAD_FILE_PATH)) {
        return;
    }

    cur_branch_ = Utils::readContentsAsString(HEAD_FILE_PATH);

    std::string branch_filepath = Utils::join(HEADS_DIR_PATH, cur_branch_);
    if (Utils::exists(branch_filepath)) {
        head_ = Utils::readContentsAsString(branch_filepath);
    }
}

void Repository::init() {
    if (Utils::exists(GITLITE_DIR_PATH)) {
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }

    Utils::createDirectories(GITLITE_DIR_PATH);
    Utils::createDirectories(OBJECTS_DIR_PATH);
    Utils::createDirectories(REFS_DIR_PATH);
    Utils::createDirectories(HEADS_DIR_PATH);

    Utils::writeContents(HEAD_FILE_PATH, "master");

    std::string message = "initial commit";
    std::map<std::string, std::string> initial_files;
    std::string initial_parent = "";
    time_t epoch_timestamp = 0;

    Commit initial_commit = Commit::create(
        message, 
        initial_files, 
        initial_parent, 
        epoch_timestamp
    );

    initial_commit.save();

    std::string master_branch_path = Utils::join(HEADS_DIR_PATH, "master");
    Utils::writeContents(master_branch_path, initial_commit.getSHA1());
}

void Repository::add(const std::string& filepath) {
    if (!Utils::exists(filepath) || !Utils::isFile(filepath)) {
        Utils::exitWithMessage("File does not exist.");
    }

    std::map<std::string, std::string> files;
    if (!head_.empty() && commits_.count(head_)) {
        files = commits_.at(head_).getTrackedFiles();
    }

    std::string cur_sha1 = Blob::fromFile(filepath).getSHA1();

    bool is_tracked = files.count(filepath) > 0;
    if (is_tracked && files.at(filepath) == cur_sha1) {
        staging_area_.unstage(filepath);
    } else {
        staging_area_.addFile(filepath);
    }
    staging_area_.save(INDEX_FILE_PATH);
}

void Repository::commit(const std::string& message) {
    if (message.empty()) {
        Utils::exitWithMessage("Please enter a commit message.");
    }

    if (staging_area_.isEmpty()) {
        Utils::exitWithMessage("No changes added to the commit.");
    }

    std::map<std::string, std::string> new_commit_files;
    if (!head_.empty() && commits_.count(head_)) {
        new_commit_files = commits_.at(head_).getTrackedFiles();
    }
    const auto& staged = staging_area_.getStagedFiles();
    for (const auto& pair : staged) {
        new_commit_files[pair.first] = pair.second;
    }
    const auto& removed = staging_area_.getRemovedFiles();
    for (const auto& filepath : removed) {
        new_commit_files.erase(filepath);
    }

    Commit new_commit = Commit::create(message, new_commit_files, head_);
    new_commit.save();

    head_ = new_commit.getSHA1();

    std::string branch_filepath = Utils::join(HEADS_DIR_PATH, cur_branch_);
    Utils::writeContents(branch_filepath, head_);

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

void Repository::log() const {
    for (std::string curr_sha = head_; !curr_sha.empty(); ) {
        if (commits_.count(curr_sha) == 0) {
            Utils::exitWithMessage("Error: Found a broken commit history link.");
            break;
        }
        const Commit& curr_commit = commits_.at(curr_sha);

        std::cout << "===" << std::endl;
        std::cout << "commit " << curr_commit.getSHA1() << std::endl;
        const auto& parents = curr_commit.getParents();
        if (parents.size() > 1) {
            std::cout << "Merge:";
            for (size_t i = 0; i < parents.size(); ++i) {
                std::cout << " " << parents[i].substr(0, 7);
            }
            std::cout << std::endl;
        }
        std::cout << "Date: " << Utils::formatTimestamp(curr_commit.getTimestamp()) << std::endl;
        std::cout << curr_commit.getMessage() << std::endl;
        std::cout << std::endl;

        if (parents.empty()) {
            break;
        }
        
        curr_sha = parents[0];
    }
}

void Repository::globalLog() const {
    for (const auto& pair : commits_) {
        const auto& curr_sha = pair.first;
        const auto& curr_commit = pair.second;

        if (commits_.count(curr_sha) == 0) {
            Utils::exitWithMessage("Error: Found a broken commit history link.");
            break;
        }

        std::cout << "===" << std::endl;
        std::cout << "commit " << curr_commit.getSHA1() << std::endl;
        const auto& parents = curr_commit.getParents();
        if (parents.size() > 1) {
            std::cout << "Merge:";
            for (size_t i = 0; i < parents.size(); ++i) {
                std::cout << " " << parents[i].substr(0, 7);
            }
            std::cout << std::endl;
        }
        std::cout << "Date: " << Utils::formatTimestamp(curr_commit.getTimestamp()) << std::endl;
        std::cout << curr_commit.getMessage() << std::endl;
        std::cout << std::endl;
    }
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