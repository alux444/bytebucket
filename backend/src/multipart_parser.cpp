#include "multipart_parser.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

// example multipart input
/*
POST /upload HTTP/1.1
Host: localhost:8080
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Length: 1234

------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="username"

john_doe
------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="description"

My profile picture upload
------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="avatar"; filename="profile.jpg"
Content-Type: image/jpeg

[BINARY JPEG DATA CONTINUES...]
------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="document"; filename="report.pdf"
Content-Type: application/pdf

[BINARY PDF DATA CONTINUES...]
------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="tags"

photo,profile,avatar
------WebKitFormBoundary7MA4YWxkTrZu0gW--

*/
namespace bytebucket
{

    std::optional<MultipartData> MultipartParser::parse(const std::string &body, const std::string &boundary)
    {
        if (boundary.empty())
        {
            return std::nullopt;
        }

        MultipartData result;

        // Create boundary markers
        std::string start_boundary = "--" + boundary;
        std::string end_boundary = "--" + boundary + "--";

        // Split by boundary
        size_t pos = 0;

        while (pos < body.length())
        {
            // Find next boundary
            size_t boundary_pos = body.find(start_boundary, pos);
            if (boundary_pos == std::string::npos)
                break;

            // Skip the boundary line
            size_t content_start = body.find("\r\n", boundary_pos);
            if (content_start == std::string::npos)
                break;
            content_start += 2; // Skip \r\n

            // Find next boundary to determine content end
            size_t next_boundary = body.find(start_boundary, content_start);
            if (next_boundary == std::string::npos)
            {
                // Check for end boundary
                next_boundary = body.find(end_boundary, content_start);
                if (next_boundary == std::string::npos)
                    break;
            }

            // Extract the part content (without the final \r\n before boundary)
            std::string part_content = body.substr(content_start, next_boundary - content_start - 2);

            // Split headers and content
            size_t header_end = part_content.find("\r\n\r\n");
            if (header_end == std::string::npos)
            {
                pos = next_boundary;
                continue;
            }

            std::string headers_section = part_content.substr(0, header_end);
            std::string content_section = part_content.substr(header_end + 4);

            // Parse headers
            auto headers = parseHeaders(headers_section);

            // Check if this is a file or regular field
            auto content_disposition = headers.find("content-disposition");
            if (content_disposition != headers.end())
            {
                // Parse Content-Disposition header
                std::string disposition = content_disposition->second;

                // Extract name
                size_t name_pos = disposition.find("name=\"");
                if (name_pos == std::string::npos)
                {
                    pos = next_boundary;
                    continue;
                }

                name_pos += 6; // Skip name="
                size_t name_end = disposition.find("\"", name_pos);
                std::string field_name = disposition.substr(name_pos, name_end - name_pos);

                // Check if it has filename (indicates file upload)
                size_t filename_pos = disposition.find("filename=\"");
                if (filename_pos != std::string::npos)
                {
                    // This is a file
                    filename_pos += 10; // Skip filename="
                    size_t filename_end = disposition.find("\"", filename_pos);
                    std::string filename = disposition.substr(filename_pos, filename_end - filename_pos);

                    std::string content_type = "application/octet-stream"; // Default
                    auto ct_iter = headers.find("content-type");
                    if (ct_iter != headers.end())
                        content_type = ct_iter->second;

                    // Create file object
                    MultipartFile file;
                    file.name = field_name;
                    file.filename = filename;
                    file.content_type = content_type;
                    file.content = std::vector<char>(content_section.begin(), content_section.end());

                    result.files.push_back(std::move(file));
                }
                else
                {
                    // This is a regular field
                    MultipartField field;
                    field.name = field_name;
                    field.value = content_section;

                    result.fields.push_back(std::move(field));
                }
            }
            pos = next_boundary;
        }

        return result;
    }

    std::string MultipartParser::extractBoundary(const std::string &content_type)
    {
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos == std::string::npos)
        {
            return "";
        }

        boundary_pos += 9; // Skip "boundary="
        std::string boundary = content_type.substr(boundary_pos);

        // Remove any trailing semicolons or whitespace
        // because content type args might not have boundary last
        // Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW; charset=utf-8
        size_t end_pos = boundary.find_first_of("; \t\r\n");
        if (end_pos != std::string::npos)
            boundary = boundary.substr(0, end_pos);

        return boundary;
    }

    std::unordered_map<std::string, std::string> MultipartParser::parseHeaders(const std::string &header_section)
    {
        std::unordered_map<std::string, std::string> headers;
        std::istringstream stream(header_section);
        std::string line;

        while (std::getline(stream, line))
        {
            // Remove \r if present
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string key = trim(line.substr(0, colon_pos));
                std::string value = trim(line.substr(colon_pos + 1));

                // Convert key to lowercase for case-insensitive lookup
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                headers[key] = value;
            }
        }

        return headers;
    }

    std::string MultipartParser::trim(const std::string &str)
    {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
        {
            return "";
        }

        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
}
