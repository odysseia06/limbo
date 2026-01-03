#include "limbo/util/FileIO.hpp"
#include "limbo/core/Assert.hpp"

#include <fstream>
#include <sstream>

namespace limbo::util {

Result<String> readFileText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        return unexpected<String>("Failed to open file: " + path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    if (file.bad()) {
        return unexpected<String>("Error reading file: " + path.string());
    }

    return buffer.str();
}

Result<std::vector<u8>> readFileBinary(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return unexpected<String>("Failed to open file: " + path.string());
    }

    auto size = file.tellg();
    if (size < 0) {
        return unexpected<String>("Failed to get file size: " + path.string());
    }

    std::vector<u8> buffer(static_cast<usize>(size));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    if (file.bad()) {
        return unexpected<String>("Error reading file: " + path.string());
    }

    return buffer;
}

Result<void, String> writeFileText(const std::filesystem::path& path, StringView content) {
    // Ensure parent directory exists
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            return unexpected<String>("Failed to create directories: " + ec.message());
        }
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return unexpected<String>("Failed to open file for writing: " + path.string());
    }

    file.write(content.data(), static_cast<std::streamsize>(content.size()));

    if (file.bad()) {
        return unexpected<String>("Error writing file: " + path.string());
    }

    return {};
}

Result<void, String> writeFileBinary(const std::filesystem::path& path, const std::vector<u8>& data) {
    // Ensure parent directory exists
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            return unexpected<String>("Failed to create directories: " + ec.message());
        }
    }

    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return unexpected<String>("Failed to open file for writing: " + path.string());
    }

    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

    if (file.bad()) {
        return unexpected<String>("Error writing file: " + path.string());
    }

    return {};
}

bool fileExists(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec;
}

bool isDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && !ec;
}

usize getFileSize(const std::filesystem::path& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        return 0;
    }
    return static_cast<usize>(size);
}

bool createDirectories(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    return !ec || std::filesystem::exists(path);
}

String getExtension(const std::filesystem::path& path) {
    return path.extension().string();
}

String getStem(const std::filesystem::path& path) {
    return path.stem().string();
}

} // namespace limbo::util
