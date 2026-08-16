#include "storage/s3_client.h"
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/DeleteObjectRequest.h>

S3Client::S3Client(
    const std::string& region,
    const std::string& bucket
)
    : bucket_(bucket)
{
    Aws::Client::ClientConfiguration config;

    config.region = region;

    client_ = Aws::S3::S3Client(config);
}


std::string S3Client::generateUploadUrl(
    const std::string& key,
    long long expirationSeconds
)
{
    return client_.GeneratePresignedUrl(
        bucket_,
        key,
        Aws::Http::HttpMethod::HTTP_PUT,
        expirationSeconds
    );
}


std::string S3Client::generateDownloadUrl(
    const std::string& key,
    long long expirationSeconds
)
{
    return client_.GeneratePresignedUrl(
        bucket_,
        key,
        Aws::Http::HttpMethod::HTTP_GET,
        expirationSeconds
    );
}

bool S3Client::objectExists(
    const std::string& key
)
{
    Aws::S3::Model::HeadObjectRequest request;

    request.SetBucket(bucket_);
    request.SetKey(key);

    auto outcome =
        client_.HeadObject(request);

    return outcome.IsSuccess();
}

bool S3Client::deleteObject(
    const std::string& key
)
{
    Aws::S3::Model::DeleteObjectRequest request;

    request.SetBucket(bucket_);
    request.SetKey(key);

    auto outcome =
        client_.DeleteObject(request);

    return outcome.IsSuccess();
}