```bash
chmod +x dev.s
./dev.sh help
./dev.sh watch-server
```

```bash
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

```bash
./bytebucket
./bytebucket_tests
```