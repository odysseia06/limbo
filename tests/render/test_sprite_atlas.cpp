#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "limbo/render/2d/SpriteAtlas.hpp"
#include "limbo/render/2d/SpriteAtlasBuilder.hpp"

#include <filesystem>

using namespace limbo;

TEST_CASE("SpriteAtlas region management", "[render][atlas]") {
    SpriteAtlas atlas;

    SECTION("starts empty") {
        REQUIRE(atlas.getRegionCount() == 0);
        REQUIRE_FALSE(atlas.isValid());
    }

    SECTION("can add and retrieve regions") {
        SpriteRegion region1;
        region1.name = "sprite1";
        region1.x = 0;
        region1.y = 0;
        region1.width = 32;
        region1.height = 32;
        region1.uvMin = {0.0f, 0.0f};
        region1.uvMax = {0.5f, 0.5f};

        atlas.addRegion(region1);

        REQUIRE(atlas.getRegionCount() == 1);
        REQUIRE(atlas.hasRegion("sprite1"));
        REQUIRE_FALSE(atlas.hasRegion("nonexistent"));

        const SpriteRegion* found = atlas.getRegion("sprite1");
        REQUIRE(found != nullptr);
        REQUIRE(found->name == "sprite1");
        REQUIRE(found->width == 32);
        REQUIRE(found->height == 32);
    }

    SECTION("can retrieve region by index") {
        SpriteRegion region1;
        region1.name = "first";
        region1.width = 16;

        SpriteRegion region2;
        region2.name = "second";
        region2.width = 32;

        atlas.addRegion(region1);
        atlas.addRegion(region2);

        REQUIRE(atlas.getRegionCount() == 2);
        REQUIRE(atlas.getRegionByIndex(0).name == "first");
        REQUIRE(atlas.getRegionByIndex(1).name == "second");
    }

    SECTION("can get all region names") {
        SpriteRegion region1;
        region1.name = "alpha";
        SpriteRegion region2;
        region2.name = "beta";

        atlas.addRegion(region1);
        atlas.addRegion(region2);

        std::vector<String> names = atlas.getRegionNames();
        REQUIRE(names.size() == 2);
        REQUIRE(names[0] == "alpha");
        REQUIRE(names[1] == "beta");
    }

    SECTION("can clear regions") {
        SpriteRegion region;
        region.name = "test";
        atlas.addRegion(region);

        REQUIRE(atlas.getRegionCount() == 1);

        atlas.clearRegions();

        REQUIRE(atlas.getRegionCount() == 0);
        REQUIRE_FALSE(atlas.hasRegion("test"));
    }
}

TEST_CASE("SpriteAtlas metadata serialization", "[render][atlas]") {
    // Create a temporary directory for testing
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "limbo_test_atlas";
    std::filesystem::create_directories(tempDir);

    SpriteAtlas atlas;
    atlas.setSize(256, 256);

    SpriteRegion region1;
    region1.name = "player";
    region1.x = 0;
    region1.y = 0;
    region1.width = 64;
    region1.height = 64;
    region1.uvMin = {0.0f, 0.0f};
    region1.uvMax = {0.25f, 0.25f};
    region1.pivot = {0.5f, 0.0f};
    region1.sourceFile = "sprites/player.png";
    atlas.addRegion(region1);

    SpriteRegion region2;
    region2.name = "enemy";
    region2.x = 64;
    region2.y = 0;
    region2.width = 32;
    region2.height = 32;
    region2.uvMin = {0.25f, 0.0f};
    region2.uvMax = {0.375f, 0.125f};
    region2.rotated = true;
    atlas.addRegion(region2);

    std::filesystem::path atlasPath = tempDir / "test.atlas";

    SECTION("can save and load metadata") {
        REQUIRE(atlas.saveMetadata(atlasPath, "test_texture.png"));

        SpriteAtlas loadedAtlas;
        String texturePath = loadedAtlas.loadMetadata(atlasPath);

        REQUIRE(texturePath == "test_texture.png");
        REQUIRE(loadedAtlas.getWidth() == 256);
        REQUIRE(loadedAtlas.getHeight() == 256);
        REQUIRE(loadedAtlas.getRegionCount() == 2);

        const SpriteRegion* player = loadedAtlas.getRegion("player");
        REQUIRE(player != nullptr);
        REQUIRE(player->width == 64);
        REQUIRE(player->height == 64);
        REQUIRE_THAT(player->pivot.x, Catch::Matchers::WithinAbs(0.5f, 0.001f));
        REQUIRE_THAT(player->pivot.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE(player->sourceFile == "sprites/player.png");

        const SpriteRegion* enemy = loadedAtlas.getRegion("enemy");
        REQUIRE(enemy != nullptr);
        REQUIRE(enemy->rotated == true);
    }

    // Cleanup
    std::filesystem::remove_all(tempDir);
}

TEST_CASE("SpriteAtlasBuilder configuration", "[render][atlas]") {
    SpriteAtlasBuilder builder;

    SECTION("starts empty") {
        REQUIRE(builder.getSpriteCount() == 0);
    }

    SECTION("can add sprites") {
        // Use non-existent paths - we'll only test the builder structure
        builder.addSprite("test1", "nonexistent1.png");
        builder.addSprite("test2", "nonexistent2.png", {0.0f, 1.0f});

        REQUIRE(builder.getSpriteCount() == 2);
    }

    SECTION("can clear sprites") {
        builder.addSprite("test", "nonexistent.png");
        REQUIRE(builder.getSpriteCount() == 1);

        builder.clear();
        REQUIRE(builder.getSpriteCount() == 0);
    }
}

TEST_CASE("AtlasBuildConfig defaults", "[render][atlas]") {
    AtlasBuildConfig config;

    REQUIRE(config.maxWidth == 4096);
    REQUIRE(config.maxHeight == 4096);
    REQUIRE(config.padding == 2);
    REQUIRE(config.allowRotation == false);
    REQUIRE(config.generateMipmaps == true);
    REQUIRE(config.powerOfTwo == true);
    REQUIRE(config.trimTransparent == false);
    REQUIRE(config.backgroundColor == 0x00000000);
}

TEST_CASE("SpriteRegion defaults", "[render][atlas]") {
    SpriteRegion region;

    REQUIRE(region.name.empty());
    REQUIRE_THAT(region.uvMin.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(region.uvMin.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(region.uvMax.x, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    REQUIRE_THAT(region.uvMax.y, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    REQUIRE(region.width == 0);
    REQUIRE(region.height == 0);
    REQUIRE_THAT(region.pivot.x, Catch::Matchers::WithinAbs(0.5f, 0.001f));
    REQUIRE_THAT(region.pivot.y, Catch::Matchers::WithinAbs(0.5f, 0.001f));
    REQUIRE(region.rotated == false);
}
