#include "database/file_repository.h"

#include <stdexcept>


FileRepository::FileRepository(
    pqxx::connection& connection
)
    : connection_(connection)
{
}


std::string FileRepository::createFile(
    const std::string& filename,
    long long fileSize,
    const std::string& contentType,
    const std::string& s3Key
) {

    pqxx::work transaction(connection_);


    pqxx::result result =
        transaction.exec_params(
            R"(
                INSERT INTO files (
                    filename,
                    file_size,
                    content_type,
                    s3_key
                )
                VALUES ($1, $2, $3, $4)
                RETURNING file_id;
            )",
            filename,
            fileSize,
            contentType,
            s3Key
        );


    transaction.commit();


    if (result.empty()) {

        throw std::runtime_error(
            "Failed to create file"
        );
    }


    return result[0]["file_id"].as<std::string>();
}


std::optional<FileMetadata>
FileRepository::findById(
    const std::string& fileId
) {

    pqxx::read_transaction transaction(
        connection_
    );


    pqxx::result result =
        transaction.exec_params(
            R"(
                SELECT
                    file_id,
                    filename,
                    file_size,
                    content_type,
                    s3_key,
                    status,
                    created_at
                FROM files
                WHERE file_id = $1;
            )",
            fileId
        );


    if (result.empty()) {
        return std::nullopt;
    }


    const auto& row = result[0];


    FileMetadata file;

    file.fileId =
        row["file_id"].as<std::string>();

    file.filename =
        row["filename"].as<std::string>();

    file.fileSize =
        row["file_size"].as<long long>();

    file.contentType =
        row["content_type"].is_null()
            ? ""
            : row["content_type"].as<std::string>();

    file.s3Key =
        row["s3_key"].as<std::string>();

    file.status =
        row["status"].as<std::string>();

    file.createdAt =
        row["created_at"].as<std::string>();


    return file;
}


void FileRepository::updateStatus(
    const std::string& fileId,
    const std::string& status
) {

    pqxx::work transaction(connection_);


    transaction.exec_params(
        R"(
            UPDATE files
            SET status = $1
            WHERE file_id = $2;
        )",
        status,
        fileId
    );


    transaction.commit();
}


void FileRepository::updateS3Key(
    const std::string& fileId,
    const std::string& s3Key
)
{
    pqxx::work transaction(
        connection_
    );


    transaction.exec_params(
        R"(
            UPDATE files
            SET s3_key = $1
            WHERE file_id = $2;
        )",
        s3Key,
        fileId
    );


    transaction.commit();
}

bool FileRepository::deleteFile(
    const std::string& fileId
)
{
    pqxx::work transaction(
        connection_
    );

    pqxx::result result =
        transaction.exec_params(
            R"(
                DELETE FROM files
                WHERE file_id = $1;
            )",
            fileId
        );

    transaction.commit();

    return !result.empty();
}