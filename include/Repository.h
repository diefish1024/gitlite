#pragma once

#include <iostream>
#include <string>
#include <map>
#include "../include/Utils.h"
#include "../include/StagingArea.h"
#include "../include/Commit.h"


class Repository {
private:
    static const std::string GITLITE_DIR_PATH;
    static const std::string OBJECTS_DIR_PATH;
    static const std::string REFS_DIR_PATH;
    static const std::string HEADS_DIR_PATH;
    static const std::string INDEX_FILE_PATH;
    static const std::string HEAD_FILE_PATH;

    StagingArea staging_area_;
    std::map<std::string, Commit> commits_;
    std::string head_;
    std::string cur_branch_;

    void loadHead();
    void loadCommits();

public:
    Repository() {
        loadHead();
        staging_area_.load(INDEX_FILE_PATH);
        loadCommits();

    }

    static const std::string& getGitliteDir();

    void init();

    void add(const std::string& filepath);
    void commit(const std::string& message);
    void rm(const std::string& filepath);

    void log() const;
    void globalLog() const;
    void find(const std::string& message) const;

    void checkoutBranch(const std::string& branchName);
    void checkoutFile(const std::string& filename);
    void checkoutFileInCommit(const std::string& commitSHA, const std::string& filename);

    void status() const;

    void branch(const std::string& branchName);
    void rmBranch(const std::string& branchName);
    void reset(const std::string& commitSHA);

    void merge(const std::string& branchName);

    void addRemote(const std::string& remoteName, const std::string& remoteURL);
    void rmRemote(const std::string& remoteName);

    void push(const std::string& remoteName, const std::string& branchName);
    void fetch(const std::string& remoteName, const std::string& branchName);
    void pull(const std::string& remoteName, const std::string& branchName);
};
