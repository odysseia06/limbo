#include <catch2/catch_test_macros.hpp>

#include <limbo/util/FileIO.hpp>

#include <filesystem>

namespace fs = std::filesystem;

// Helper to create a unique temp directory for tests
class TempDirectory {
public:
    TempDirectory() {
        m_path = fs::temp_directory_path() / "limbo_test" / std::to_string(reinterpret_cast<uintptr_t>(this));
        fs::create_directories(m_path);
    }

    ~TempDirectory() {
        std::error_code ec;
        fs::remove_all(m_path, ec);
    }

    const fs::path& path() const { return m_path; }

private:
    fs::path m_path;
};

TEST_CASE("File I/O text operations", "[util][fileio]") {
    TempDirectory tempDir;

    SECTION("Write and read text file") {
        auto filePath = tempDir.path() / "test.txt";
        limbo::String content = "Hello, Limbo Engine!\nLine 2\nLine 3";

        auto writeResult = limbo::util::writeFileText(filePath, content);
        REQUIRE(writeResult.has_value());

        auto readResult = limbo::util::readFileText(filePath);
        REQUIRE(readResult.has_value());
        REQUIRE(readResult.value() == content);
    }

    SECTION("Read non-existent file returns error") {
        auto filePath = tempDir.path() / "nonexistent.txt";

        auto result = limbo::util::readFileText(filePath);
        REQUIRE(!result.has_value());
        REQUIRE(result.error().find("Failed to open") != limbo::String::npos);
    }

    SECTION("Write creates parent directories") {
        auto filePath = tempDir.path() / "subdir" / "nested" / "test.txt";
        limbo::String content = "nested content";

        auto writeResult = limbo::util::writeFileText(filePath, content);
        REQUIRE(writeResult.has_value());
        REQUIRE(fs::exists(filePath));

        auto readResult = limbo::util::readFileText(filePath);
        REQUIRE(readResult.has_value());
        REQUIRE(readResult.value() == content);
    }

    SECTION("Write empty file") {
        auto filePath = tempDir.path() / "empty.txt";

        auto writeResult = limbo::util::writeFileText(filePath, "");
        REQUIRE(writeResult.has_value());

        auto readResult = limbo::util::readFileText(filePath);
        REQUIRE(readResult.has_value());
        REQUIRE(readResult.value().empty());
    }
}

TEST_CASE("File I/O binary operations", "[util][fileio]") {
    TempDirectory tempDir;

    SECTION("Write and read binary file") {
        auto filePath = tempDir.path() / "test.bin";
        std::vector<limbo::u8> data = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};

        auto writeResult = limbo::util::writeFileBinary(filePath, data);
        REQUIRE(writeResult.has_value());

        auto readResult = limbo::util::readFileBinary(filePath);
        REQUIRE(readResult.has_value());
        REQUIRE(readResult.value() == data);
    }

    SECTION("Read binary non-existent file returns error") {
        auto filePath = tempDir.path() / "nonexistent.bin";

        auto result = limbo::util::readFileBinary(filePath);
        REQUIRE(!result.has_value());
    }

    SECTION("Write empty binary file") {
        auto filePath = tempDir.path() / "empty.bin";
        std::vector<limbo::u8> data;

        auto writeResult = limbo::util::writeFileBinary(filePath, data);
        REQUIRE(writeResult.has_value());

        auto readResult = limbo::util::readFileBinary(filePath);
        REQUIRE(readResult.has_value());
        REQUIRE(readResult.value().empty());
    }
}

TEST_CASE("File utility functions", "[util][fileio]") {
    TempDirectory tempDir;

    SECTION("fileExists") {
        auto existingFile = tempDir.path() / "exists.txt";
        limbo::util::writeFileText(existingFile, "content");

        REQUIRE(limbo::util::fileExists(existingFile));
        REQUIRE(!limbo::util::fileExists(tempDir.path() / "nonexistent.txt"));
    }

    SECTION("isDirectory") {
        auto subDir = tempDir.path() / "subdir";
        fs::create_directories(subDir);

        auto file = tempDir.path() / "file.txt";
        limbo::util::writeFileText(file, "content");

        REQUIRE(limbo::util::isDirectory(subDir));
        REQUIRE(limbo::util::isDirectory(tempDir.path()));
        REQUIRE(!limbo::util::isDirectory(file));
        REQUIRE(!limbo::util::isDirectory(tempDir.path() / "nonexistent"));
    }

    SECTION("getFileSize") {
        auto file = tempDir.path() / "sized.txt";
        limbo::String content = "12345"; // 5 bytes
        limbo::util::writeFileText(file, content);

        REQUIRE(limbo::util::getFileSize(file) == 5);
        REQUIRE(limbo::util::getFileSize(tempDir.path() / "nonexistent.txt") == 0);
    }

    SECTION("createDirectories") {
        auto nested = tempDir.path() / "a" / "b" / "c";

        REQUIRE(limbo::util::createDirectories(nested));
        REQUIRE(fs::exists(nested));
        REQUIRE(fs::is_directory(nested));

        // Should succeed even if already exists
        REQUIRE(limbo::util::createDirectories(nested));
    }

    SECTION("getExtension") {
        REQUIRE(limbo::util::getExtension("file.txt") == ".txt");
        REQUIRE(limbo::util::getExtension("path/to/file.png") == ".png");
        REQUIRE(limbo::util::getExtension("file.tar.gz") == ".gz");
        REQUIRE(limbo::util::getExtension("noextension") == "");
    }

    SECTION("getStem") {
        REQUIRE(limbo::util::getStem("file.txt") == "file");
        REQUIRE(limbo::util::getStem("path/to/file.png") == "file");
        REQUIRE(limbo::util::getStem("file.tar.gz") == "file.tar");
        REQUIRE(limbo::util::getStem("noextension") == "noextension");
    }
}
