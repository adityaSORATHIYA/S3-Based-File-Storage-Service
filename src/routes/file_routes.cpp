#include "routes/file_routes.h"

#include <random>
#include <sstream>
#include <stdexcept>


static std::string buildS3Key(
    const std::string& fileId,
    const std::string& filename
)
{
    return "objects/" + fileId + "/" + filename;
}


void registerFileRoutes(
    crow::SimpleApp& app,
    FileRepository& fileRepository,
    S3Client& s3Client
)
{
    CROW_ROUTE(app, "/files/upload")
        .methods(crow::HTTPMethod::POST)
    (
        [&fileRepository, &s3Client](
            const crow::request& request
        )
        {
            try {

                auto body =
                    crow::json::load(request.body);


                if (!body) {

                    return crow::response(
                        400,
                        "Invalid JSON"
                    );
                }


                if (
                    !body.has("filename")
                    ||
                    !body.has("size")
                    ||
                    !body.has("content_type")
                ) {

                    return crow::response(
                        400,
                        "Missing required fields"
                    );
                }


                std::string filename =
                    body["filename"].s();

                long long fileSize =
                    body["size"].i();

                std::string contentType =
                    body["content_type"].s();


                if (filename.empty()) {

                    return crow::response(
                        400,
                        "Filename cannot be empty"
                    );
                }


                if (fileSize < 0) {

                    return crow::response(
                        400,
                        "File size cannot be negative"
                    );
                }


                /*
                 * We don't know the file ID yet.
                 *
                 * First create a temporary S3 key.
                 *
                 * We need the UUID from PostgreSQL.
                 */

                std::string temporaryKey =
                    "pending/" + filename;


                /*
                 * Create database record.
                 *
                 * PostgreSQL generates UUID.
                 */

                std::string fileId =
                    fileRepository.createFile(
                        filename,
                        fileSize,
                        contentType,
                        temporaryKey
                    );


                /*
                 * Build final S3 key.
                 */

                std::string s3Key =
                    buildS3Key(
                        fileId,
                        filename
                    );


                /*
                 * Update S3 key in PostgreSQL.
                 *
                 * We'll add this repository method next.
                 */


                fileRepository.updateS3Key(
                    fileId,
                    s3Key
                );

                std::string uploadUrl =
                    s3Client.generateUploadUrl(
                        s3Key,
                        900
                    );


                crow::json::wvalue response;

                response["file_id"] =
                    fileId;

                response["upload_url"] =
                    uploadUrl;

                response["s3_key"] =
                    s3Key;

                response["expires_in"] =
                    900;

                response["status"] =
                    "PENDING";


                return crow::response(
                    201,
                    response
                );

            }
            catch (
                const std::exception& e
            ) {

                crow::json::wvalue response;

                response["error"] =
                    e.what();

                return crow::response(
                    500,
                    response
                );
            }
        }
    );

    CROW_ROUTE(app, "/files/<string>/complete")
        .methods(crow::HTTPMethod::POST)
    (
        [&fileRepository, &s3Client](
            const crow::request&,
            const std::string& fileId
        )
        {
            try {

                auto file =
                    fileRepository.findById(fileId);

                if (!file.has_value()) {

                    return crow::response(
                        404,
                        R"({"error":"File not found"})"
                    );
                }

                if (
                    !s3Client.objectExists(
                        file->s3Key
                    )
                ) {

                    return crow::response(
                        400,
                        R"({"error":"File not uploaded to S3"})"
                    );
                }

                fileRepository.updateStatus(
                    fileId,
                    "COMPLETED"
                );

                crow::json::wvalue response;

                response["file_id"] =
                    fileId;

                response["status"] =
                    "COMPLETED";

                return crow::response(
                    200,
                    response
                );
            }
            catch (
                const std::exception& e
            ) {

                crow::json::wvalue response;

                response["error"] =
                    e.what();

                return crow::response(
                    500,
                    response
                );
            }
        }
    );


    CROW_ROUTE(app, "/files/<string>/download")
        .methods(crow::HTTPMethod::GET)
    (
        [&fileRepository, &s3Client](
            const std::string& fileId
        )
        {
            try {

                auto file =
                    fileRepository.findById(fileId);

                if (!file.has_value()) {

                    return crow::response(
                        404,
                        R"({"error":"File not found"})"
                    );
                }

                if (file->status != "COMPLETED") {

                    return crow::response(
                        400,
                        R"({"error":"File is not ready for download"})"
                    );
                }

                std::string downloadUrl =
                    s3Client.generateDownloadUrl(
                        file->s3Key,
                        900
                    );

                crow::json::wvalue response;

                response["file_id"] =
                    file->fileId;

                response["filename"] =
                    file->filename;

                response["download_url"] =
                    downloadUrl;

                response["expires_in"] =
                    900;

                return crow::response(
                    200,
                    response
                );
            }
            catch (
                const std::exception& e
            ) {

                crow::json::wvalue response;

                response["error"] =
                    e.what();

                return crow::response(
                    500,
                    response
                );
            }
        }
    );

    CROW_ROUTE(app, "/files/<string>")
        .methods(crow::HTTPMethod::DELETE)
    (
        [&fileRepository, &s3Client](
            const std::string& fileId
        )
        {
            try {

                auto file =
                    fileRepository.findById(fileId);

                if (!file.has_value()) {

                    return crow::response(
                        404,
                        R"({"error":"File not found"})"
                    );
                }

                bool deletedFromS3 =
                    s3Client.deleteObject(
                        file->s3Key
                    );

                if (!deletedFromS3) {

                    return crow::response(
                        500,
                        R"({"error":"Failed to delete file from S3"})"
                    );
                }

                fileRepository.deleteFile(
                    fileId
                );

                crow::json::wvalue response;

                response["file_id"] =
                    fileId;

                response["status"] =
                    "DELETED";

                return crow::response(
                    200,
                    response
                );
            }
            catch (
                const std::exception& e
            ) {

                crow::json::wvalue response;

                response["error"] =
                    e.what();

                return crow::response(
                    500,
                    response
                );
            }
        }
    );
}