# multi part form

- type of http content type that allows for requests to upload binary data, blobs and text
- good use case for this project because i want to upload data and tag it with metadata for searching etc.

# cors preflight

- normally javascript code blocks requests to a diff origin
- before allowing a cross origin request, browser will send a preflight request
  - OPTIONS type that includes the origin url
  - we return a response with the CORS headers
  - browser checks if the CORS headers matches what it needs. if it does, send the actual request
  - if the headers are missing or incorrect, the request is cancelled by the browser
  - all enforced by the browser, not the server
- in node when using something like express, can just do `app.use(cors());`
  - in c++, gotta do it manually (lol)

# unique ptr deleter pattern

```cpp
std::unique_ptr<sqlite3, SQLiteDeleter> db;
std::unique_ptr<T, D>
```

when it's destroyed, it should call `SQLiteDeleter::operator()(sqlite3*)`

# sqlite execution

```cpp
sqlite3_bind_text(stmt, 1, name.data(), static_cast<int>(name.size()), SQLITE_STATIC);
// SQLITE_STATIC = memory in name.data() is guaranteed until after sqlite is complete with it
// if not, use SQLITE_TRANSIENT - will immediately make a copy of the string so that it's safe to modify the reference right after the call
```
