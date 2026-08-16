#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

#include <string>

class S3Client {
public:

    S3Client(
        const std::string& region,
        const std::string& bucket
    );

    std::string generateUploadUrl(
        const std::string& key,
        long long expirationSeconds = 900
    );

    std::string generateDownloadUrl(
        const std::string& key,
        long long expirationSeconds = 900
    );

    bool objectExists(
        const std::string& key
    );

    bool deleteObject(
        const std::string& key
    );
private:

    std::string bucket_;

    Aws::S3::S3Client client_;
};