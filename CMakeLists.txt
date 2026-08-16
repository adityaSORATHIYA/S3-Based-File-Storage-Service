cmake_minimum_required(VERSION 3.20)

project(LargeFileStorage)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(AWSSDK REQUIRED COMPONENTS s3)

find_package(libpqxx CONFIG REQUIRED)

find_package(Crow REQUIRED)


add_executable(storage_server

    src/main.cpp

    src/database/postgres.cpp
    src/database/file_repository.cpp

    src/storage/s3_client.cpp

    src/routes/file_routes.cpp
)


target_include_directories(
    storage_server
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)


target_link_libraries(
    storage_server
    PRIVATE

        aws-cpp-sdk-s3
        aws-cpp-sdk-core
        /opt/homebrew/opt/libpqxx/lib/libpqxx.dylib

        Crow::Crow
)