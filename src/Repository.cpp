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
    staging_area_.addFile(filepath);
    if (is_tracked && files.at(filepath) == cur_sha1) {
        staging_area_.unstage(filepath);
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

void Repository::find(const std::string& message) const {
    bool is_find = false;
    for (const auto& pair : commits_) {
        const auto& curr_commit = pair.second;

        if (curr_commit.getMessage() == message) {
            std::cout << curr_commit.getSHA1() << std::endl;
            is_find = true;
        }
    }
    if (!is_find) {
        Utils::exitWithMessage("Found no commit with that message.");
    }
}

void Repository::checkoutFile(const std::string &filename) {
    checkoutFileInCommit(head_, filename);
}

void Repository::checkoutFileInCommit(const std::string &commitSHA, const std::string &filename) {
    std::string resSHA = "";
    if (commitSHA.length() == 40 && commits_.count(commitSHA)) {
        resSHA = commitSHA;
    } else {
        for (const auto& [fullSHA, commit] : commits_) {
            if (fullSHA.find(commitSHA) == 0) {
                resSHA = fullSHA;
                break;
            }
        }
    }
    if (resSHA.empty()) {
        Utils::exitWithMessage("No commit with that id exists.");
    }
    const Commit& commit = commits_.at(resSHA);
    const std::map<std::string, std::string>& trackedFiles = commit.getTrackedFiles();

    auto it = trackedFiles.find(filename);
    if (it == trackedFiles.end()) {
        Utils::exitWithMessage("File does not exist in that commit.");
    }

    Blob blob = Blob::load(it->second);
    Utils::writeContents(filename, blob.getContent());
}

void Repository::checkoutBranch(const std::string &branchName) {
    std::string branchFilePath = Utils::join(HEADS_DIR_PATH, branchName);
    if (!Utils::exists(branchFilePath)) {
        Utils::exitWithMessage("No such branch exists.");
    }
    if (branchName == cur_branch_) {
        Utils::exitWithMessage("No need to checkout the current branch.");
    }
    std::string targetCommitSHA = Utils::readContentsAsString(branchFilePath);
    Commit currentCommit = commits_.at(head_); 
    Commit targetCommit = commits_.at(targetCommitSHA);
    std::map<std::string, std::string> currentFiles = currentCommit.getTrackedFiles();
    std::map<std::string, std::string> targetFiles = targetCommit.getTrackedFiles();

    std::vector<std::string> workDirFiles = Utils::plainFilenamesIn(".");
    for (const auto& filename : workDirFiles) {
        bool isTrackedCur = currentFiles.count(filename);
        bool isTrackedTar = targetFiles.count(filename);
        if (isTrackedCur && isTrackedTar) {
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }
    
    for (const auto& pair : currentFiles) {
        const std::string& filename = pair.first;
        if (targetFiles.find(filename) == targetFiles.end()) {
            Utils::restrictedDelete(filename);
        }
    }
    for (const auto& pair : targetFiles) {
        const std::string& filename = pair.first;
        const std::string& blobSHA = pair.second;

        Blob blob = Blob::load(blobSHA);
        Utils::writeContents(filename, blob.getContent());
    }

    head_ = targetCommitSHA;
    cur_branch_ = branchName;

    Utils::writeContents(HEAD_FILE_PATH, cur_branch_);
    
    staging_area_.clear();
    staging_area_.save(INDEX_FILE_PATH);
}

void Repository::status() const {
    std::cout << "=== Branches ===" << std::endl;
    std::vector<std::string> branches = Utils::plainFilenamesIn(HEADS_DIR_PATH);
    for (const auto& branch : branches) {
        if (branch == cur_branch_) {
            std::cout << "*" << branch << std::endl;
        } else {
            std::cout << branch << std::endl;
        }
    }
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
}