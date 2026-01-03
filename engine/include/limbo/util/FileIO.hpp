#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <vector>
#include <filesystem>

namespace limbo::util {

// Read entire file as string
// Returns Result with file contents or error message
LIMBO_API Result<String> readFileText(const std::filesystem::path& path);

// Read entire file as binary data
// Returns Result with file bytes or error message
LIMBO_API Result<std::vector<u8>> readFileBinary(const std::filesystem::path& path);

// Write string to file (overwrites if exists)
// Returns Result<void> indicating success or error message
LIMBO_API Result<void, String> writeFileText(const std::filesystem::path& path, StringView content);

// Write binary data to file (overwrites if exists)
// Returns Result<void> indicating success or error message
LIMBO_API Result<void, String> writeFileBinary(const std::filesystem::path& path, const std::vector<u8>& data);

// Check if file exists
LIMBO_API bool fileExists(const std::filesystem::path& path);

// Check if path is a directory
LIMBO_API bool isDirectory(const std::filesystem::path& path);

// Get file size in bytes, returns 0 if file doesn't exist
LIMBO_API usize getFileSize(const std::filesystem::path& path);

// Create directories recursively (like mkdir -p)
// Returns true if directories were created or already exist
LIMBO_API bool createDirectories(const std::filesystem::path& path);

// Get the file extension (e.g., ".txt", ".png")
LIMBO_API String getExtension(const std::filesystem::path& path);

// Get filename without extension
LIMBO_API String getStem(const std::filesystem::path& path);

} // namespace limbo::util
