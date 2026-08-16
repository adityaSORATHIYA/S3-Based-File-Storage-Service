#pragma once

#include <crow.h>

#include "database/file_repository.h"
#include "storage/s3_client.h"

void registerFileRoutes(
    crow::SimpleApp& app,
    FileRepository& fileRepository,
    S3Client& s3Client
);