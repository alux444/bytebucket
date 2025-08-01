PRAGMA foreign_keys = ON;

-- TODO: Virtual table with partial name matching

-- FOLDERS table
CREATE TABLE folders (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    parent_id INTEGER,
    FOREIGN KEY (parent_id) REFERENCES folders(id) ON DELETE SET NULL
);

-- FILES table
CREATE TABLE files (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    folder_id INTEGER NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    size INTEGER,
    content_type TEXT,
    FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE
);

-- Indexes
CREATE INDEX idx_files_folder_id ON files(folder_id);
CREATE INDEX idx_folders_parent_id ON folders(parent_id);
CREATE INDEX idx_folder_name ON folders(name);
CREATE INDEX idx_files_name ON files(name);
CREATE INDEX idx_files_content_type ON files(content_type);

-- TAGS table
CREATE TABLE tags (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

-- Index on tags.name (implicitly created by UNIQUE, but good to declare explicitly)
CREATE INDEX idx_tags_name ON tags(name);

-- FILE_TAGS join table
CREATE TABLE file_tags (
    file_id INTEGER NOT NULL,
    tag_id INTEGER NOT NULL,
    PRIMARY KEY (file_id, tag_id),
    FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
);

-- FILE_METADATA key-value table
CREATE TABLE file_metadata (
    file_id INTEGER NOT NULL,
    key TEXT NOT NULL,
    value TEXT,
    PRIMARY KEY (file_id, key),
    FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE
);
