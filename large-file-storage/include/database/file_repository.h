#pragma once

#include "models/file_metadata.h"

#include <pqxx/pqxx>

#include <optional>
#include <string>

class FileRepository {
public:

    explicit FileRepository(
        pqxx::connection& connection
    );

    std::string createFile(
        const std::string& filename,
        long long fileSize,
        const std::string& contentType,
        const std::string& s3Key
    );

    std::optional<FileMetadata> findById(
        const std::string& fileId
    );

    void updateStatus(
        const std::string& fileId,
        const std::string& status
    );

    void updateS3Key(
        const std::string& fileId,
        const std::string& s3Key
    );

    bool deleteFile(
        const std::string& fileId
    );

private:

    pqxx::connection& connection_;
};