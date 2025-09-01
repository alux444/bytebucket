# ByteBucket TODOS

## 🌐 HTTP Server

- [x] Set up a minimal HTTP server with Boost.Beast
- [x] Implement routing for:
  - [x] `GET /` — serve landing page or status
  - [x] `GET /health` — health check endpoint
  - [x] `POST /folder` — create folder endpoint
  - [x] `POST /upload` — file upload handler (multipart/form-data)
  - [x] `GET /download/{file_id}` — file download handler
  - [x] `GET /tags` — get all available tags
  - [x] `POST /tags` — create new tag
  - [x] `POST /files/{file_id}/tags` — add tag to file
  - [x] `POST /files/{file_id}/metadata` — add metadata to file
- [ ] Handle concurrent connections using `std::thread`
- [ ] Add CORS headers for frontend integration

## 📂 Folder Management System

- [x] Create folder structure in database (hierarchical)
- [x] Implement `POST /folder` endpoint for creating folders
  - [x] Support root folders (no parent)
  - [x] Support nested folders (with parent_id)
- [x] Implement folder listing endpoints:
  - [x] `GET /folder` — get root folder contents (files and subfolders)
  - [x] `GET /folder/{folder_id}` — get specific folder contents (files and subfolders)
- [ ] Implement folder operations:
  - [ ] `PUT /folder/{folder_id}` — rename folder
  - [ ] `DELETE /folder/{folder_id}` — delete folder (and contents)
  - [ ] `POST /folder/{folder_id}/move` — move folder to different parent

## 📤 File Upload / Download

- [x] Implement multipart/form-data parsing for uploads
- [x] Save uploaded files into `storage/` folder with unique IDs
- [x] Generate and save file metadata (filename, size, timestamp, content_type)
- [x] Link files to folders in database
- [x] Implement file upload to specific folder:
  - [x] `POST /upload` with optional `folder_id` parameter
- [x] Serve file downloads via streaming:
  - [x] `GET /download/{file_id}` — download file by file ID
  - [x] Include proper content headers (filename, content-type)
- [ ] File operations:
  - [ ] `PUT /files/{file_id}` — rename file
  - [ ] `DELETE /files/{file_id}` — delete file
  - [ ] `POST /files/{file_id}/move` — move file to different folder

## 🗃 Database Schema & Operations

- [x] SQLite database with tables:
  - [x] `folders` (id, name, parent_id, created_at)
  - [x] `files` (id, name, folder_id, size, content_type, storage_id, created_at, updated_at)
  - [x] `tags` (id, name) — for file tagging
  - [x] `file_tags` (file_id, tag_id) — many-to-many relationship
  - [x] `file_metadata` (file_id, key, value) — custom metadata
- [x] Implement database operations:
  - [x] `insertFolder()` — create new folder
  - [x] `addFile()` — add file record to database
  - [x] `getFolderById()` — get folder details
  - [x] `getFoldersByParent()` — list folders by parent
  - [x] `getFilesByFolder()` — list files in folder
  - [x] `getFileById()` — get file details
  - [x] `getAllTags()` — retrieve all tags
  - [x] `createTag()` — create new tag
  - [x] `addTagToFile()` — associate tag with file
  - [x] `addMetadataToFile()` — add key-value metadata to file
  - [ ] `updateFolder()` — rename folder
  - [ ] `deleteFolder()` — remove folder and cascade
  - [ ] `updateFile()` — rename file
  - [ ] `deleteFile()` — remove file record
  - [ ] `moveFile()` — change file's folder
  - [ ] `moveFolder()` — change folder's parent

## 🏷️ Tags & Metadata System

- [x] Implement tag management:
  - [x] `GET /tags` — get all available tags
  - [x] `POST /tags` — create new tag
  - [x] `POST /files/{file_id}/tags` — add tag to file
- [x] Implement metadata management:
  - [x] `POST /files/{file_id}/metadata` — add custom metadata to file
- [x] Database operations for tags and metadata:
  - [x] `getAllTags()` — retrieve all tags
  - [x] `createTag()` — create new tag
  - [x] `addTagToFile()` — associate tag with file
  - [x] `addMetadataToFile()` — add key-value metadata to file
- [ ] Additional tag/metadata operations:
  - [ ] `DELETE /files/{file_id}/tags/{tag_name}` — remove tag from file
  - [ ] `PUT /files/{file_id}/metadata/{key}` — update metadata value
  - [ ] `DELETE /files/{file_id}/metadata/{key}` — remove metadata key

## 🌍 File Browser Interface (Frontend)

- [x] Basic React frontend structure with TypeScript
- [x] API client with proper type definitions
- [ ] Create Google Drive-like interface with:
  - [ ] **Folder Tree Navigation** — sidebar with expandable folder tree
  - [ ] **Breadcrumb Navigation** — current path display
  - [ ] **Main Content Area** — files and folders in current directory
  - [ ] **File Grid/List View** — toggle between grid and list layouts
  - [ ] **Upload Zone** — drag-and-drop file upload
- [ ] Folder operations:
  - [ ] Create new folder button
  - [ ] Right-click context menu (rename, delete, move)
  - [ ] Folder double-click to navigate
- [ ] File operations:
  - [ ] File preview/thumbnails
  - [ ] Download files
  - [ ] Right-click context menu (rename, delete, move, share)
  - [ ] File details panel (size, type, date, metadata)
- [ ] Bulk operations:
  - [ ] Multi-select files/folders
  - [ ] Bulk move, delete, download

## 🔄 Advanced File Operations

- [ ] **Cut/Copy/Paste System**:
  - [ ] `POST /clipboard/cut` — mark items for moving
  - [ ] `POST /clipboard/copy` — mark items for copying
  - [ ] `POST /folder/{folder_id}/paste` — paste items to folder
- [ ] **Search & Filter**:
  - [ ] `GET /search?q={query}&folder_id={id}` — search files/folders
  - [ ] Filter by file type, date range, size
  - [ ] Search within folder hierarchy
- [ ] **File Sharing**:
  - [ ] `POST /files/{file_id}/share` — create shareable link
  - [ ] `GET /shared/{share_token}` — access shared file
  - [ ] Link expiration and password protection

## 🧪 Testing

- [x] Integrate Catch2 testing framework
- [x] Write tests for:
  - [x] Database operations (folders, files, tags, metadata)
  - [x] Basic HTTP endpoints (health, root)
  - [x] File upload/download endpoints
  - [x] Folder management endpoints
  - [x] Tag and metadata endpoints
  - [x] Error handling and edge cases
- [x] Comprehensive test coverage (6 test cases, 69 assertions)
- [ ] Add integration tests for frontend-backend communication
- [ ] Add performance tests for large file operations

## 🔐 User Authentication System (Future)

- [ ] Create `POST /register` endpoint for user signup
- [ ] Create `POST /login` endpoint for authentication
- [ ] Implement session management with JWTs
- [ ] Add user ownership to folders and files:
  - [ ] Add `user_id` to folders and files tables
  - [ ] Filter operations by user ownership
- [ ] User permissions and sharing:
  - [ ] Folder sharing between users
  - [ ] Read/write permissions system

## 🛡️ Security & Stability

- [ ] **File System Security**:
  - [ ] Sanitize filenames and paths to prevent directory traversal
  - [ ] Validate file types and sizes
  - [ ] Implement storage quotas per user
- [ ] **API Security**:
  - [ ] Rate limiting on upload/download endpoints
  - [ ] CSRF protection
  - [ ] Input validation for all endpoints
- [ ] **Infrastructure**:
  - [ ] Use HTTPS (OpenSSL with Boost.Beast or reverse proxy)
  - [ ] Implement comprehensive logging
  - [ ] Graceful shutdown with cleanup
  - [ ] Database connection pooling

## 🧠 Future Enhancements

### 🟢 Easy

- [x] **File Metadata & Tagging**:
  - [x] Add custom tags to files
  - [x] Custom key-value metadata for files
  - [ ] File description and notes
  - [ ] File favorites/bookmarks
- [ ] **UI Improvements**:
  - [ ] File type icons and thumbnails
  - [ ] Keyboard shortcuts (Ctrl+C, Ctrl+V, Delete, etc.)
  - [ ] Progress indicators for uploads/downloads
  - [ ] Recent files and frequently accessed
- [ ] **Storage Management**:
  - [ ] Storage usage statistics
  - [ ] Trash bin with restore functionality
  - [ ] Auto-cleanup of old trash items

### 🟡 Medium

- [ ] **Advanced File Operations**:
  - [ ] File versioning system
  - [ ] Duplicate file detection and deduplication
  - [ ] Batch file operations (rename, convert)
  - [ ] ZIP folder download
- [ ] **Collaboration Features**:
  - [ ] Real-time collaboration on folders
  - [ ] Activity feed and notifications
  - [ ] Comments on files and folders
  - [ ] User groups and team folders
- [ ] **Search & Discovery**:
  - [ ] Full-text search inside documents
  - [ ] Smart folders based on criteria
  - [ ] File recommendation system

### 🔴 Hard

- [ ] **Sync & Backup**:
  - [ ] Multi-device synchronization
  - [ ] Offline mode with conflict resolution
  - [ ] Automated backups with encryption
  - [ ] Delta sync for large files
- [ ] **Advanced Security**:
  - [ ] End-to-end encryption for sensitive files
  - [ ] Two-factor authentication
  - [ ] Audit logs and compliance features
  - [ ] Advanced permission system (ACLs)
- [ ] **Performance & Scale**:
  - [ ] Distributed storage backend
  - [ ] CDN integration for downloads
  - [ ] Database sharding for large datasets
  - [ ] Caching layer for metadata operations

## 📊 API Endpoints Summary

### ✅ Implemented Endpoints

- `GET /` — Landing page/status
- `GET /health` — Health check
- `GET /folder` — Get root folder contents
- `GET /folder/{folder_id}` — Get specific folder contents
- `POST /folder` — Create new folder
- `POST /upload` — Upload files (with optional folder_id)
- `GET /download/{file_id}` — Download file
- `GET /tags` — Get all tags
- `POST /tags` — Create new tag
- `POST /files/{file_id}/tags` — Add tag to file
- `POST /files/{file_id}/metadata` — Add metadata to file

### 🚧 Planned Endpoints

- File operations: rename, delete, move
- Folder operations: rename, delete, move
- Advanced tag/metadata operations
- Search and filtering
- User authentication
- File sharing
