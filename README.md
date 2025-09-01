# ByteBucket

<div align="center">
  <img src="assets/logo.png" alt="ByteBucket Logo" width="200"/>
  
  **A self-hosted file browser with a modern web interface**
  
  ![Build Status](https://github.com/username/bytebucket/workflows/Build%20and%20Test/badge.svg)
  ![License](https://img.shields.io/badge/license-MIT-blue.svg)
  ![C++](https://img.shields.io/badge/C++-17-blue.svg)
  ![React](https://img.shields.io/badge/React-18-blue.svg)
</div>

## 🚀 Features

- **📁 Folder Management**
- **📤 File Upload/Download**
- **🌐 Web UI**
- **🔧 RESTful API**
- **📊 File Metadata**

## 📁 Project Structure

```
bytebucket/
│
├── backend/            # C++ backend (Boost.Beast + SQLite)
│   ├── include/        # Header files
│   ├── src/            # Source code
│   ├── tests/          # Catch2 unit tests
│   ├── storage/        # File storage directory
│   └── CMakeLists.txt  # Build configuration
│
├── frontend/           # React frontend
│   ├── src/            # React source code
│   └── ...             # Standard React project structure
│
└── assets/             # Project assets (logos, etc.)
```

## 🛠️ Tech Stack

### Backend

- **C++17** compatible compiler (g++, clang++)
- **CMake** 3.15+
- **vcpkg** package manager
- **Boost.Beast** (HTTP server)
- **SQLite3** (database)
- **Catch2** (testing framework)

### Frontend Dependencies

- **TypeScript**
- **Node**
- **React**

## 📊 Current Status

**🚧 Work in Progress** - ByteBucket is currently under active development.

### ✅ Completed Features

- HTTP server with Boost.Beast
- SQLite database schema and operations
- Folder creation and management
- File upload with multipart/form-data
- File download functionality
- Comprehensive test suite
- Development tooling

### 🚧 In Progress / TODO

See [`backend/TODO.md`](backend/TODO.md) for detailed development roadmap

## 🏗️ Architecture

- **Backend**: C++ with Boost.Beast HTTP server, SQLite database
- **Frontend**: React with TypeScript, REST API client
- **Storage**: File system blob storage with database metadata
- **Database**: SQLite with tables for folders, files, tags, and metadata

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run tests (`./dev.sh test`)
5. Commit your changes (`git commit -m 'Add amazing feature'`)
6. Push to the branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

<div align="center">
  Built with ❤️ using C++ and React
</div>
