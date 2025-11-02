#pragma once
#include <iostream>
#include <string>

#include "Utils.h"

class Blob {
private:
    std::string content_;
    std::string sha1_;

    Blob(const std::string& cont, const std::string& sha1)
        : content_(cont), sha1_(sha1) {}

public:
    static Blob fromContent(const std::string& content);
    static Blob fromFile(const std::string& filepath);

    static Blob load(const std::string& sha1);
    bool save();

    std::string getSHA1() const { return sha1_; }
    std::string getContent() const { return content_; }
};