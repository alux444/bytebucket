# ByteBucket TODOS

C++ self-hosted storage using **Boost.Beast** as the HTTP library.

---

## 🛠️ Environment

- Install a C++17 (or later) compiler (e.g., g++, clang++)
- Install [CMake](https://cmake.org/)
- Install a C++ package manager like [vcpkg](https://github.com/microsoft/vcpkg)
- Install Boost libraries (especially Boost.Beast) via your package manager  
  vcpkg:
  ```bash
  vcpkg install boost-beast boost-system boost-thread catch2
  ```

---

## 📁 Project Structure

```
bytebucket/

├── backend/            # C++ backend source code and headers
│   ├── data/           # metadata storage (db / jsons)
│   ├── include/        # backend header files
│   ├── src/            # backend source code
│   ├── storage/        # uploaded files storage
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
  - [x] `POST /upload` — file upload handler
  - [x] `GET /download?file_id=xyz` — file download handler
- [ ] Handle concurrent connections using `std::thread` or Boost ASIO's async model

## 📤 File Upload / Download

- [ ] Implement multipart/form-data parsing for uploads
- [ ] Save uploaded files into `storage/` folder
- [ ] Generate and save file metadata (filename, size, timestamp, user ID)
- [ ] Serve file downloads via streaming to clients

## 🔐 User Authentication System

- [ ] Create `POST /register` endpoint for user signup
- [ ] Create `POST /login` endpoint for authentication
- [ ] Securely store passwords (bcrypt or SHA256 with salt)
- [ ] Implement session management with tokens or JWTs
- [ ] Add middleware to validate sessions for protected routes

## 🗃 Metadata Storage

- [ ] Choose database for metadata storage
- [ ] Define tables:
  - [ ] `users` (id, username, password_hash)
  - [ ] `files` (id, user_id, name, size, path, timestamp)
- [ ] Implement CRUD operations for users and files
- [ ] Wrap database operations in safe C++ abstractions

## 🧪 Testing

- [ ] Integrate a unit testing framework (e.g., doctest or Catch2)
- [ ] Write tests for:
  - [ ] Upload endpoint
  - [ ] Download endpoint
  - [ ] Authentication system
  - [ ] Database operations

## 🌍 Web Interface (Frontend)

- [ ] Create webapp for
  - [ ] Upload files (drag-and-drop)
  - [ ] List user files
  - [ ] Download/delete files
- [ ] Serve frontend static files from `/public`
- [ ] Use Fetch API to communicate with backend endpoints

## 🛡️ Security & Stability

- [ ] Use HTTPS (set up OpenSSL with Boost.Beast or run behind a reverse proxy like Nginx)
- [ ] Sanitize filenames and paths to prevent directory traversal
- [ ] Enforce file upload size limits
- [ ] Implement logging for uploads, downloads, and errors
- [ ] Ensure graceful shutdown saves metadata state

## 🧠 Future Enhancements

### 🟢 Easy

- [ ] Trash bin / recycle bin functionality
- [ ] Search by filename, date, or tags
- [ ] Folder management
- [ ] Trash auto-cleanup
- [ ] Activity logs & audit trail
- [ ] Notification system
- [ ] Storage quotas & usage monitoring

### 🟡 Medium

- [ ] File sharing with expiring, password-protected links
- [ ] User group permissions (editor, viewer, commenter)
- [ ] Deduplication & compression
- [ ] Full-text search — Enable searching inside documents and file metadata
- [ ] Encrypted backups — Automatically back up metadata and files with encryption

### 🔴 Hard

- [ ] File versioning system
- [ ] End-to-end encryption for user files
- [ ] Multi-device syncing
- [ ] Conflict resolution
- [ ] Offline mode
