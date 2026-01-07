#include <catch2/catch_test_macros.hpp>

#include <limbo/assets/AssetId.hpp>
#include <limbo/core/UUID.hpp>
#include <unordered_set>

TEST_CASE("UUID basic functionality", "[core][uuid]") {
    SECTION("Default constructed UUID is null") {
        limbo::UUID const uuid;
        REQUIRE(uuid.isNull());
        REQUIRE(!uuid.isValid());
    }

    SECTION("Generated UUID is valid") {
        limbo::UUID uuid = limbo::UUID::generate();
        REQUIRE(!uuid.isNull());
        REQUIRE(uuid.isValid());
    }

    SECTION("Generated UUIDs are unique") {
        limbo::UUID uuid1 = limbo::UUID::generate();
        limbo::UUID uuid2 = limbo::UUID::generate();
        REQUIRE(uuid1 != uuid2);
    }

    SECTION("UUID to string and back") {
        limbo::UUID uuid = limbo::UUID::generate();
        limbo::String str = uuid.toString();
        limbo::UUID parsed = limbo::UUID::fromString(str);
        REQUIRE(uuid == parsed);
    }

    SECTION("UUID string format is correct") {
        limbo::UUID uuid = limbo::UUID::generate();
        limbo::String str = uuid.toString();
        // Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
        REQUIRE(str.size() == 36);
        REQUIRE(str[8] == '-');
        REQUIRE(str[13] == '-');
        REQUIRE(str[18] == '-');
        REQUIRE(str[23] == '-');
    }
}

TEST_CASE("AssetId basic functionality", "[assets][assetid]") {
    SECTION("Default constructed AssetId is invalid") {
        limbo::AssetId const id;
        REQUIRE(!id.isValid());
    }

    SECTION("Generated AssetId is valid") {
        limbo::AssetId id = limbo::AssetId::generate();
        REQUIRE(id.isValid());
    }

    SECTION("AssetId from path is valid") {
        limbo::AssetId const id("textures/player.png");
        REQUIRE(id.isValid());
    }

    SECTION("Same path produces same ID") {
        limbo::AssetId id1("textures/player.png");
        limbo::AssetId id2("textures/player.png");
        REQUIRE(id1 == id2);
    }

    SECTION("Different paths produce different IDs") {
        limbo::AssetId id1("textures/player.png");
        limbo::AssetId id2("textures/enemy.png");
        REQUIRE(id1 != id2);
    }

    SECTION("Invalid static factory returns invalid ID") {
        auto id = limbo::AssetId::invalid();
        REQUIRE(!id.isValid());
    }

    SECTION("AssetId to string and back") {
        limbo::AssetId id = limbo::AssetId::generate();
        limbo::String str = id.toString();
        limbo::AssetId parsed = limbo::AssetId::fromString(str);
        REQUIRE(id == parsed);
    }
}

TEST_CASE("AssetId comparison operators", "[assets][assetid]") {
    limbo::AssetId id1 = limbo::AssetId::generate();
    limbo::AssetId id2 = limbo::AssetId::generate();
    limbo::AssetId id3 = id1;  // Copy

    SECTION("Equality") {
        REQUIRE(id1 == id3);
        REQUIRE(!(id1 == id2));
    }

    SECTION("Inequality") {
        REQUIRE(id1 != id2);
        REQUIRE(!(id1 != id3));
    }
}

TEST_CASE("AssetId can be used in unordered containers", "[assets][assetid]") {
    std::unordered_set<limbo::AssetId> ids;

    limbo::AssetId const id1("texture1.png");
    limbo::AssetId const id2("texture2.png");
    limbo::AssetId const id3("texture1.png");  // Same as id1

    ids.insert(id1);
    ids.insert(id2);
    ids.insert(id3);  // Should not insert (duplicate)

    REQUIRE(ids.size() == 2);
    REQUIRE(ids.count(id1) == 1);
    REQUIRE(ids.count(id2) == 1);
}
