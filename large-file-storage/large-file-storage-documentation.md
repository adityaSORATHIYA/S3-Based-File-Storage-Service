# Large File Storage System

## 1. Project Overview

A simple large-file storage service built with:

- **C++** — application/backend
- **Crow** — HTTP REST API
- **PostgreSQL** — file metadata
- **Amazon S3** — actual file storage
- **AWS SDK for C++** — S3 integration
- **CMake** — build system

The current design intentionally keeps retrieval simple:

- No chunking/multipart upload
- No resumable uploads
- No iTree/index tree
- File retrieval is by `file_id`
- S3 stores the actual file
- PostgreSQL stores metadata

---

# 2. Architecture

```text
                         Client
                           |
                           | HTTP
                           v
                  +-------------------+
                  |   Crow C++ Server  |
                  +-------------------+
                     |             |
                     |             |
                     v             v
              +-----------+    +-----------+
              | PostgreSQL|    |    S3     |
              | Metadata  |    | File Data |
              +-----------+    +-----------+
                     ^
                     |
              FileRepository
                     |
                  S3Client
```

## Responsibilities

### PostgreSQL

Stores metadata:

```text
file_id
filename
file_size
content_type
s3_key
status
created_at
```

### S3

Stores the actual file bytes.

Example:

```text
objects/
└── 69cda7ef-3b9c-4908-80f7-20783cb2b9f2/
    └── hello.txt
```

### C++ Server

Connects PostgreSQL and S3 and exposes the REST APIs.

---

# 3. Project Structure

```text
large-file-storage/
│
├── CMakeLists.txt
│
├── include/
│   ├── config.h
│   │
│   ├── models/
│   │   ├── file_metadata.h
│   │   └── upload_session.h
│   │
│   ├── database/
│   │   ├── postgres.h
│   │   ├── file_repository.h
│   │   └── upload_repository.h
│   │
│   └── storage/
│       └── s3_client.h
│
├── src/
│   ├── main.cpp
│   │
│   ├── database/
│   │   ├── postgres.cpp
│   │   ├── file_repository.cpp
│   │   └── upload_repository.cpp
│   │
│   ├── storage/
│   │   └── s3_client.cpp
│   │
│   └── routes/
│       └── file_routes.cpp
│
├── sql/
│   └── schema.sql
│
├── tests/
│
└── README.md
```

---

# 4. Current API List

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/health` | Check server |
| GET | `/health/db` | Check PostgreSQL |
| POST | `/files/upload` | Create file metadata + generate S3 upload URL |
| POST | `/files/{file_id}/complete` | Verify S3 upload and mark file completed |
| GET | `/files/{file_id}/download` | Generate S3 download URL |
| DELETE | `/files/{file_id}` | Delete S3 object and PostgreSQL metadata |

---

# 5. Environment Variables

These variables must be available in the same terminal from which the server is started.

## PostgreSQL

Your local PostgreSQL role is:

```text
sorathiyaadityavijaybhai
```

Set:

```bash
export DATABASE_URL="postgresql://sorathiyaadityavijaybhai@localhost:5432/large_file_storage"
```

If your PostgreSQL setup later requires a password, use:

```bash
export DATABASE_URL="postgresql://USERNAME:PASSWORD@localhost:5432/large_file_storage"
```

## AWS Region

The S3 bucket used by this project is in Mumbai:

```text
ap-south-1
```

Set:

```bash
export AWS_REGION="ap-south-1"
```

## S3 Bucket

Current bucket:

```text
large-file-storage-system-2026
```

Set:

```bash
export S3_BUCKET="large-file-storage-system-2026"
```

## Verify

Run:

```bash
echo $DATABASE_URL
echo $AWS_REGION
echo $S3_BUCKET
```

Expected:

```text
postgresql://sorathiyaadityavijaybhai@localhost:5432/large_file_storage
ap-south-1
large-file-storage-system-2026
```

> AWS access keys are NOT stored in these environment variables. The AWS CLI/AWS SDK obtains credentials from your AWS configuration/credential chain. Never commit AWS secret credentials to Git.

---

# 6. AWS CLI Setup

The AWS CLI is already installed.

Check:

```bash
aws --version
```

Configure the IAM credentials provided by the bucket owner:

```bash
aws configure
```

Enter:

```text
AWS Access Key ID: <IAM access key>
AWS Secret Access Key: <IAM secret key>
Default region name: ap-south-1
Default output format: json
```

The IAM user must have appropriate S3 permissions, including:

```text
s3:PutObject
s3:GetObject
s3:DeleteObject
```

For listing the bucket, it also needs:

```text
s3:ListBucket
```

Test credentials:

```bash
aws sts get-caller-identity
```

Test S3 access:

```bash
aws s3 ls s3://large-file-storage-system-2026 --region ap-south-1
```

Note: `ListBucket` may be denied even when `PutObject` works. That is an IAM permission issue, not necessarily an authentication problem.

---

# 7. PostgreSQL Setup

Start PostgreSQL if necessary:

```bash
brew services start postgresql@14
```

Create the database:

```bash
createdb large_file_storage
```

Enter PostgreSQL:

```bash
psql large_file_storage
```

Run the schema:

```sql
\i sql/schema.sql
```

Check tables:

```sql
\dt
```

The schema includes:

```text
files
upload_sessions
upload_parts
```

The current application uses the `files` table for the simple upload/retrieval flow.

Exit:

```sql
\q
```

Check PostgreSQL roles:

```bash
psql -d large_file_storage -c "\du"
```

---

# 8. Build

From the project root:

```bash
cd ~/large-file-storage
```

Configure:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build
```

Expected:

```text
[100%] Built target storage_server
```

The project uses the AWS SDK for C++ S3 component.

---

# 9. Run the Server

First set environment variables:

```bash
export DATABASE_URL="postgresql://sorathiyaadityavijaybhai@localhost:5432/large_file_storage"
export AWS_REGION="ap-south-1"
export S3_BUCKET="large-file-storage-system-2026"
```

Then:

```bash
./build/storage_server
```

Keep this terminal running.

Use a second terminal for API testing.

---

# 10. API: Health Check

## Request

```bash
curl http://localhost:8080/health
```

## Response

```json
{
  "status": "ok"
}
```

Purpose:

Confirms that the HTTP server is running.

---

# 11. API: Database Health

## Request

```bash
curl http://localhost:8080/health/db
```

## Response

```json
{
  "database": "postgresql",
  "status": "ok"
}
```

Purpose:

Confirms that the C++ server can connect to PostgreSQL.

---

# 12. API: Start Upload

## Endpoint

```http
POST /files/upload
```

## Request

```bash
curl -X POST http://localhost:8080/files/upload \
  -H "Content-Type: application/json" \
  -d '{
    "filename": "hello.txt",
    "size": 12,
    "content_type": "text/plain"
  }'
```

## Response

Example:

```json
{
  "status": "PENDING",
  "s3_key": "objects/69cda7ef-3b9c-4908-80f7-20783cb2b9f2/hello.txt",
  "upload_url": "https://large-file-storage-system-2026.s3.ap-south-1.amazonaws.com/objects/69cda7ef-3b9c-4908-80f7-20783cb2b9f2/hello.txt?...",
  "expires_in": 900,
  "file_id": "69cda7ef-3b9c-4908-80f7-20783cb2b9f2"
}
```

## What happens

```text
Client
  |
  | POST /files/upload
  v
C++ Server
  |
  +--> PostgreSQL: create metadata
  |
  +--> S3Client: generate presigned PUT URL
  |
  v
Client receives file_id + upload_url
```

The `file_id` is generated by the server.

The human user does not need to remember the UUID. A frontend/client should store the association between the displayed filename and `file_id`.

---

# 13. Upload Actual File to S3

The previous API only creates metadata and generates a presigned URL.

The actual file is uploaded using the returned `upload_url`.

Example:

```bash
curl -X PUT \
  --upload-file hello.txt \
  "PASTE_THE_COMPLETE_UPLOAD_URL_HERE"
```

The complete URL must contain all S3 signature parameters.

Do not replace any part of the URL with `...`.

The URL expires after:

```text
900 seconds = 15 minutes
```

---

# 14. API: Complete Upload

## Endpoint

```http
POST /files/{file_id}/complete
```

Example:

```bash
curl -X POST \
  http://localhost:8080/files/69cda7ef-3b9c-4908-80f7-20783cb2b9f2/complete
```

## Response

```json
{
  "file_id": "69cda7ef-3b9c-4908-80f7-20783cb2b9f2",
  "status": "COMPLETED"
}
```

## What happens

```text
Client
  |
  | POST /files/{id}/complete
  v
C++ Server
  |
  +--> PostgreSQL: find file
  |
  +--> S3: HeadObject
  |
  +--> If object exists:
  |        PostgreSQL status = COMPLETED
  |
  v
Response
```

This prevents the database from becoming `COMPLETED` merely because a presigned URL was generated.

---

# 15. API: Retrieve File by ID

## Endpoint

```http
GET /files/{file_id}/download
```

Example:

```bash
curl \
  http://localhost:8080/files/69cda7ef-3b9c-4908-80f7-20783cb2b9f2/download
```

## Response

```json
{
  "file_id": "69cda7ef-3b9c-4908-80f7-20783cb2b9f2",
  "filename": "hello.txt",
  "download_url": "https://large-file-storage-system-2026.s3.ap-south-1.amazonaws.com/objects/...",
  "expires_in": 900
}
```

The backend performs:

```text
file_id
   |
   v
PostgreSQL
   |
   +--> s3_key
   |
   v
S3Client
   |
   +--> presigned GET URL
   |
   v
Client
```

The actual file bytes are downloaded directly from S3.

---

# 16. Download the Actual File

After receiving `download_url`:

```bash
curl -o download.txt "PASTE_DOWNLOAD_URL_HERE"
```

Verify:

```bash
cat download.txt
```

Check size:

```bash
wc -c download.txt
```

For the `hello.txt` test:

```text
12 download.txt
```

---

# 17. API: Delete File

## Endpoint

```http
DELETE /files/{file_id}
```

Example:

```bash
curl -X DELETE \
  http://localhost:8080/files/69cda7ef-3b9c-4908-80f7-20783cb2b9f2
```

## Response

```json
{
  "status": "DELETED",
  "file_id": "69cda7ef-3b9c-4908-80f7-20783cb2b9f2"
}
```

## What happens

```text
DELETE /files/{id}
        |
        v
PostgreSQL findById()
        |
        v
Get s3_key
        |
        v
Delete S3 object
        |
        v
Delete PostgreSQL metadata
        |
        v
DELETED
```

The current implementation deletes S3 first and then deletes the PostgreSQL record.

---

# 18. Verify Deletion in S3

```bash
aws s3api head-object \
  --bucket large-file-storage-system-2026 \
  --key objects/69cda7ef-3b9c-4908-80f7-20783cb2b9f2/hello.txt \
  --region ap-south-1
```

Expected after deletion:

```text
An error occurred (404) when calling the HeadObject operation: Not Found
```

---

# 19. Verify Deletion in PostgreSQL

```bash
psql large_file_storage -c \
"SELECT * FROM files WHERE file_id = '69cda7ef-3b9c-4908-80f7-20783cb2b9f2';"
```

Expected:

```text
(0 rows)
```

---

# 20. Complete Upload Flow

The complete upload process is:

```text
                    Client
                       |
                       | 1. POST /files/upload
                       v
                +--------------+
                | C++ / Crow   |
                +--------------+
                   |         |
                   |         |
                   v         v
              PostgreSQL     S3
              metadata       |
                   |          |
                   |     presigned URL
                   |          |
                   +----------+
                       |
                       v
                  Client gets
                upload_url + ID
                       |
                       | 2. PUT file
                       v
                      S3
                       |
                       | 3. POST /complete
                       v
                  C++ Server
                       |
                       | HeadObject
                       v
                      S3
                       |
                       v
              PostgreSQL status
                 PENDING
                    |
                    v
                COMPLETED
```

---

# 21. Complete Retrieval Flow

```text
Client
  |
  | GET /files/{file_id}/download
  v
C++ Server
  |
  | SELECT metadata FROM files
  v
PostgreSQL
  |
  | s3_key
  v
C++ S3Client
  |
  | Generate presigned GET URL
  v
Client
  |
  | GET presigned URL
  v
S3
  |
  v
File bytes
```

---

# 22. Complete Delete Flow

```text
Client
  |
  | DELETE /files/{file_id}
  v
C++ Server
  |
  | findById()
  v
PostgreSQL
  |
  | s3_key
  v
S3 DeleteObject
  |
  v
S3 object removed
  |
  v
PostgreSQL DELETE
  |
  v
Metadata removed
```

---

# 23. Current Database Model

The main table is:

```text
files
```

Important fields:

```text
file_id
filename
file_size
content_type
s3_key
status
created_at
```

Example:

```text
file_id:
69cda7ef-3b9c-4908-80f7-20783cb2b9f2

filename:
hello.txt

file_size:
12

content_type:
text/plain

s3_key:
objects/69cda7ef-3b9c-4908-80f7-20783cb2b9f2/hello.txt

status:
COMPLETED
```

---

# 24. File ID / UUID

The UUID is an internal identifier generated by the backend.

The user does not need to manually remember it.

A frontend can maintain:

```text
hello.txt
    |
    +--> file_id = 69cda7ef-...
```

The UI can simply show:

```text
hello.txt       Download    Delete
```

while internally using:

```http
GET /files/69cda7ef-.../download
DELETE /files/69cda7ef-...
```

---

# 25. S3 Object Naming

The current S3 key format is:

```text
objects/{file_id}/{filename}
```

Example:

```text
objects/69cda7ef-3b9c-4908-80f7-20783cb2b9f2/hello.txt
```

This gives every uploaded file a unique directory-like prefix.

---

# 26. Important Current Limitations

The current system intentionally does NOT implement:

- Multipart/chunked uploads
- Resumable uploads
- iTree/index-tree retrieval
- User authentication
- User-specific file ownership
- File listing API
- Sharing/access control

The current retrieval mechanism is intentionally simple:

```text
file_id
  |
  v
PostgreSQL
  |
  v
s3_key
  |
  v
S3 presigned GET URL
```

---

# 27. Useful Manual S3 Commands

## Upload directly with AWS CLI

```bash
aws s3 cp hello.txt \
s3://large-file-storage-system-2026/objects/test/hello.txt \
--region ap-south-1
```

## Check object

```bash
aws s3api head-object \
--bucket large-file-storage-system-2026 \
--key objects/test/hello.txt \
--region ap-south-1
```

## Delete object

```bash
aws s3api delete-object \
--bucket large-file-storage-system-2026 \
--key objects/test/hello.txt \
--region ap-south-1
```

---

# 28. Recommended Development Workflow

Every new terminal session:

```bash
cd ~/large-file-storage

export DATABASE_URL="postgresql://sorathiyaadityavijaybhai@localhost:5432/large_file_storage"
export AWS_REGION="ap-south-1"
export S3_BUCKET="large-file-storage-system-2026"
```

Build:

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/storage_server
```

Open a second terminal for API calls.

---

# 29. Quick API Test Checklist

## Health

```bash
curl http://localhost:8080/health
```

Expected:

```json
{"status":"ok"}
```

## Database

```bash
curl http://localhost:8080/health/db
```

Expected:

```json
{"database":"postgresql","status":"ok"}
```

## Upload

```bash
curl -X POST http://localhost:8080/files/upload \
  -H "Content-Type: application/json" \
  -d '{
    "filename": "hello.txt",
    "size": 12,
    "content_type": "text/plain"
  }'
```

## Upload file to S3

```bash
curl -X PUT \
  --upload-file hello.txt \
  "PRESIGNED_UPLOAD_URL"
```

## Complete

```bash
curl -X POST \
  http://localhost:8080/files/FILE_ID/complete
```

## Retrieve

```bash
curl \
  http://localhost:8080/files/FILE_ID/download
```

## Download

```bash
curl -o download.txt "PRESIGNED_DOWNLOAD_URL"
```

## Delete

```bash
curl -X DELETE \
  http://localhost:8080/files/FILE_ID
```

---

# 30. End-to-End Status

The currently verified system supports:

```text
Server health                  ✅
PostgreSQL connection          ✅
AWS authentication             ✅
S3 connection                  ✅
Create upload metadata         ✅
Generate presigned PUT URL     ✅
Upload file to S3              ✅
Verify S3 object               ✅
Mark upload COMPLETED          ✅
Retrieve file by ID            ✅
Generate presigned GET URL     ✅
Download file                  ✅
Delete S3 object               ✅
Delete PostgreSQL metadata     ✅
```

The core architecture is therefore:

```text
                LARGE FILE STORAGE

                       Client
                         |
                         v
                  C++ REST Server
                   /           \
                  /             \
                 v               v
          PostgreSQL             S3
          metadata            file bytes
                 \               /
                  \             /
                   \           /
                    S3Client
```

This is the current simple, working architecture.
