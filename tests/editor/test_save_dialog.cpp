#include "EditorUtils.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

using limbo::editor::buildEditorWindowTitle;
using limbo::editor::sanitizeSceneFilename;

// --- Filename sanitization ---

TEST_CASE("sanitizeSceneFilename basic inputs", "[editor]") {
    SECTION("plain name gets .json appended") {
        REQUIRE(sanitizeSceneFilename("myScene") == "myScene.json");
    }

    SECTION("name with .json extension is unchanged") {
        REQUIRE(sanitizeSceneFilename("myScene.json") == "myScene.json");
    }

    SECTION("empty input is rejected") {
        REQUIRE(sanitizeSceneFilename("").empty());
    }
}

TEST_CASE("sanitizeSceneFilename path traversal", "[editor]") {
    SECTION("relative path traversal is stripped") {
        REQUIRE(sanitizeSceneFilename("../../evil") == "evil.json");
    }

    SECTION("deep relative path traversal is stripped") {
        REQUIRE(sanitizeSceneFilename("../../../etc/passwd") == "passwd.json");
    }

    SECTION("dot-dot alone is rejected") {
        REQUIRE(sanitizeSceneFilename("..").empty());
    }

    SECTION("single dot is rejected") {
        REQUIRE(sanitizeSceneFilename(".").empty());
    }
}

TEST_CASE("sanitizeSceneFilename absolute paths", "[editor]") {
    SECTION("absolute path is stripped to filename") {
        REQUIRE(sanitizeSceneFilename("/absolute/path") == "path.json");
    }

    SECTION("Windows-style absolute path is stripped to filename") {
        REQUIRE(sanitizeSceneFilename("C:\\Users\\test\\scene") == "scene.json");
    }
}

TEST_CASE("sanitizeSceneFilename extension handling", "[editor]") {
    SECTION("wrong case extension gets .json appended") {
        REQUIRE(sanitizeSceneFilename("name.JSON") == "name.JSON.json");
    }

    SECTION("other extensions get .json appended") {
        REQUIRE(sanitizeSceneFilename("scene.txt") == "scene.txt.json");
    }

    SECTION("traversal with .json extension is stripped but extension kept") {
        REQUIRE(sanitizeSceneFilename("../../evil.json") == "evil.json");
    }
}

// --- Window title generation ---

TEST_CASE("buildEditorWindowTitle untitled scene", "[editor]") {
    SECTION("unmodified") {
        REQUIRE(buildEditorWindowTitle({}, false) == "Limbo Editor - Untitled");
    }

    SECTION("modified") {
        REQUIRE(buildEditorWindowTitle({}, true) == "Limbo Editor - Untitled*");
    }
}

TEST_CASE("buildEditorWindowTitle with scene path", "[editor]") {
    std::filesystem::path const scenePath = "assets/scenes/level1.json";

    SECTION("unmodified") {
        REQUIRE(buildEditorWindowTitle(scenePath, false) == "Limbo Editor - level1.json");
    }

    SECTION("modified") {
        REQUIRE(buildEditorWindowTitle(scenePath, true) == "Limbo Editor - level1.json*");
    }
}
