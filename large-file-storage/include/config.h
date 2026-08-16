#pragma once

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace Config {

inline std::string getEnv(
    const char* name
) {
    const char* value =
        std::getenv(name);

    if (!value) {
        throw std::runtime_error(
            std::string(
                "Missing environment variable: "
            ) + name
        );
    }

    return std::string(value);
}


inline std::string databaseUrl() {

    return getEnv("DATABASE_URL");
}


inline std::string awsRegion() {

    return getEnv("AWS_REGION");
}


inline std::string s3Bucket() {

    return getEnv("S3_BUCKET");
}

}