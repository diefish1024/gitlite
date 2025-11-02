#pragma once
#include <bits/types/time_t.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

class Commit {
private:
    std::map<std::string, std::string> file_blob_;
    std::vector<std::string> parent_commits_; // merge -> multiple parents
    std::string message_;
    time_t timestamp_;
    std::string sha1_;

    
    Commit(const std::map<std::string, std::string>& files,
        const std::vector<std::string>& parents,
        const std::string& message,
        time_t timestamp,
        const std::string& sha1)
        : file_blob_(files), parent_commits_(parents), message_(message),
        timestamp_(timestamp), sha1_(sha1) {}
        
        std::string Seralize() const;
public:
    Commit() = default;

    std::string getMessage() const { return message_; }
    time_t getTimestamp() const { return timestamp_; }
    std::vector<std::string> getParents() const { return parent_commits_; }
    
    static Commit create(const std::string& message,
        const std::map<std::string, std::string>& files,
        const std::string& parent = "",
        const time_t timestamp = time(nullptr)
    );
    static Commit create(const std::string& message,
        const std::map<std::string, std::string>& files,
        const std::vector<std::string>& parents,
        const time_t timestamp = time(nullptr)
    );
    static Commit load(const std::string& filepath);

    void save();

    void addFile(const std::string& filepath, const std::string& blob_sha);

    std::string getSHA1() const { return sha1_; }
    const std::map<std::string, std::string>& getTrackedFiles() const { return file_blob_; }
};