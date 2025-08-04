# ByteBucket TODOS

C++ self-hosted file browser using **Boost.Beast** as the HTTP library.

---

## 🛠️ Environment

- Install a C++17 (or later) compiler (e.g., g++, clang++)
- Install [CMake](https://cmake.org/)
- Install a C++ package manager like [vcpkg](https://github.com/microsoft/vcpkg)
- Install Boost libraries (especially Boost.Beast) via your package manager  
  vcpkg:
  ```bash
  vcpkg install boost-beast boost-system boost-thread catch2
  vcpkg install sqlite3
  ```

---

## 📁 Project Structure

```
bytebucket/

├── backend/            # C++ backend source code and headers
│   ├── data/           # metadata storage (SQLite database)
│   ├── include/        # backend header files
│   ├── src/            # backend source code
│   ├── storage/        # uploaded files storage (blob storage)
│   ├── tests/          # backend tests
│   └── CMakeLists.txt  # backend build configuration
│
└── frontend/           # React frontend application
    ├── src/            # React source code
    └── (React project files)

```

## 🌐 HTTP Server (Boost.Beast)

- [x] Set up a minimal HTTP server with Boost.Beast
- [x] Implement routing for:
  - [x] `GET /` — serve landing page or status
  - [x] `GET /health` — health check endpoint
  - [x] `POST /folder` — create folder endpoint
  - [ ] `POST /file` — file upload handler
  - [x] `GET /download/{file_id}` — file download handler
- [ ] Handle concurrent connections using `std::thread`
- [ ] Add CORS headers for frontend integration

## 📂 Folder Management System

- [x] Create folder structure in database (hierarchical)
- [x] Implement `POST /folder` endpoint for creating folders
  - [x] Support root folders (no parent)
  - [x] Support nested folders (with parent_id)
- [ ] Implement folder listing endpoints:
  - [ ] `GET /folders` — list all root folders
  - [ ] `GET /folders/{folder_id}` — get folder details and contents
  - [ ] `GET /folders/{folder_id}/children` — list subfolders and files
- [ ] Implement folder operations:
  - [ ] `PUT /folders/{folder_id}` — rename folder
  - [ ] `DELETE /folders/{folder_id}` — delete folder (and contents)
  - [ ] `POST /folders/{folder_id}/move` — move folder to different parent

## 📤 File Upload / Download

- [x] Implement multipart/form-data parsing for uploads
- [x] Save uploaded files into `storage/` folder with unique IDs
- [x] Generate and save file metadata (filename, size, timestamp, content_type)
- [x] Link files to folders in database
- [ ] Implement file upload to specific folder:
  - [ ] `POST /folders/{folder_id}/upload` — upload file to folder
- [ ] Serve file downloads via streaming:
  - [ ] `GET /download/{storage_id}` — download file by storage ID
  - [ ] Include proper content headers (filename, content-type)
- [ ] File operations:
  - [ ] `PUT /files/{file_id}` — rename file
  - [ ] `DELETE /files/{file_id}` — delete file
  - [ ] `POST /files/{file_id}/move` — move file to different folder

## 🗃 Database Schema & Operations

- [x] SQLite database with tables:
  - [x] `folders` (id, name, parent_id, created_at)
  - [x] `files` (id, name, folder_id, size, content_type, storage_id, created_at, updated_at)
  - [x] `tags` (id, name) — for future file tagging
  - [x] `file_tags` (file_id, tag_id) — many-to-many relationship
  - [x] `file_metadata` (file_id, key, value) — custom metadata
- [x] Implement database operations:
  - [x] `insertFolder()` — create new folder
  - [x] `addFile()` — add file record to database
  - [ ] `getFolderById()` — get folder details
  - [ ] `getFoldersByParent()` — list folders by parent
  - [ ] `getFilesByFolder()` — list files in folder
  - [ ] `updateFolder()` — rename folder
  - [ ] `deleteFolder()` — remove folder and cascade
  - [ ] `updateFile()` — rename file
  - [ ] `deleteFile()` — remove file record
  - [ ] `moveFile()` — change file's folder
  - [ ] `moveFolder()` — change folder's parent

## 🌍 File Browser Interface (Frontend)

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
  - [ ] `POST /folders/{folder_id}/paste` — paste items to folder
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
  - [x] Database operations (folders, files)
  - [x] Basic HTTP endpoints
  - [ ] File upload/download endpoints
  - [ ] Folder management endpoints
  - [ ] File operations endpoints
  - [ ] Error handling and edge cases

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

- [ ] **File Metadata & Tagging**:
  - [ ] Add custom tags to files
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
