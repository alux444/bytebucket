# ByteBucket Backend

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

```bash
chmod +x dev.s
./dev.sh help
./dev.sh watch-server
```
