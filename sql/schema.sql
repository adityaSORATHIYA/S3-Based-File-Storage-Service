CREATE EXTENSION IF NOT EXISTS pgcrypto;


CREATE TABLE IF NOT EXISTS files (
    file_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),

    filename TEXT NOT NULL,

    file_size BIGINT NOT NULL CHECK (file_size >= 0),

    content_type TEXT,

    s3_key TEXT NOT NULL UNIQUE,

    status TEXT NOT NULL DEFAULT 'PENDING',

    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);


CREATE TABLE IF NOT EXISTS upload_sessions (
    upload_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),

    file_id UUID NOT NULL
        REFERENCES files(file_id)
        ON DELETE CASCADE,

    s3_upload_id TEXT NOT NULL,

    part_size BIGINT NOT NULL
        CHECK (part_size > 0),

    total_parts INTEGER NOT NULL
        CHECK (total_parts > 0),

    status TEXT NOT NULL DEFAULT 'UPLOADING',

    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);


CREATE TABLE IF NOT EXISTS upload_parts (
    upload_id UUID NOT NULL
        REFERENCES upload_sessions(upload_id)
        ON DELETE CASCADE,

    part_number INTEGER NOT NULL
        CHECK (part_number > 0),

    etag TEXT,

    status TEXT NOT NULL DEFAULT 'PENDING',

    PRIMARY KEY (upload_id, part_number)
);


CREATE INDEX IF NOT EXISTS idx_files_created_at
    ON files(created_at);


CREATE INDEX IF NOT EXISTS idx_files_status
    ON files(status);


CREATE INDEX IF NOT EXISTS idx_upload_sessions_file_id
    ON upload_sessions(file_id);


CREATE INDEX IF NOT EXISTS idx_upload_parts_upload_id
    ON upload_parts(upload_id);