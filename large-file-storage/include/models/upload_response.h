#pragma once

#include <string>

struct UploadResponse {
    std::string fileId;
    std::string uploadUrl;
    std::string s3Key;
    long long expiresIn;
};