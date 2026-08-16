#include "config.h"

#include "database/postgres.h"
#include "database/file_repository.h"

#include <aws/core/Aws.h>

#include <crow.h>

#include "routes/file_routes.h"
#include "storage/s3_client.h"
#include <iostream>


int main() {

    Aws::SDKOptions awsOptions;

    Aws::InitAPI(awsOptions);


    try {

        /*
         * PostgreSQL
         */

        std::string databaseUrl =
            Config::databaseUrl();


        Postgres postgres(
            databaseUrl
        );


        FileRepository fileRepository(
            postgres.connection()
        );

        S3Client s3Client(
            Config::awsRegion(),
            Config::s3Bucket()
        );

        /*
         * HTTP server
         */

        crow::SimpleApp app;

        registerFileRoutes(
            app,
            fileRepository,
            s3Client
        );

        CROW_ROUTE(app, "/health")
        .methods(crow::HTTPMethod::GET)
        (
            [] {

                crow::json::wvalue response;

                response["status"] =
                    "ok";

                return response;
            }
        );


        CROW_ROUTE(app, "/health/db")
        .methods(crow::HTTPMethod::GET)
        (
            [&postgres] {

                try {

                    pqxx::read_transaction transaction(
                        postgres.connection()
                    );


                    pqxx::result result =
                        transaction.exec(
                            "SELECT 1;"
                        );


                    if (
                        !result.empty()
                        &&
                        result[0][0].as<int>() == 1
                    ) {

                        crow::json::wvalue response;

                        response["status"] =
                            "ok";

                        response["database"] =
                            "postgresql";

                        return response;
                    }

                }
                catch (
                    const std::exception& e
                ) {

                    crow::json::wvalue response;

                    response["status"] =
                        "error";

                    response["message"] =
                        e.what();

                    return response;
                }


                crow::json::wvalue response;

                response["status"] =
                    "error";

                return response;
            }
        );


        app
            .port(8080)
            .multithreaded()
            .run();

    }
    catch (
        const std::exception& e
    ) {

        std::cerr
            << "Fatal error: "
            << e.what()
            << '\n';


        Aws::ShutdownAPI(
            awsOptions
        );


        return 1;
    }


    Aws::ShutdownAPI(
        awsOptions
    );


    return 0;
}