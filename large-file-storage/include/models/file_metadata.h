#pragma once

#include <string>

struct FileMetadata {
    std::string fileId;

    std::string filename;

    long long fileSize = 0;

    std::string contentType;

    std::string s3Key;

    std::string status;

    std::string createdAt;
};