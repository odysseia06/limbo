#include <catch2/catch_test_macros.hpp>

#include <limbo/assets/AssetId.hpp>
#include <unordered_set>

TEST_CASE("AssetId basic functionality", "[assets][assetid]") {
    SECTION("Default constructed AssetId is invalid") {
        limbo::AssetId const id;
        REQUIRE(!id.isValid());
        REQUIRE(id.value() == 0);
    }

    SECTION("AssetId from value is valid") {
        limbo::AssetId const id(12345);
        REQUIRE(id.isValid());
        REQUIRE(id.value() == 12345);
    }

    SECTION("AssetId from path is valid") {
        limbo::AssetId const id("textures/player.png");
        REQUIRE(id.isValid());
        REQUIRE(id.value() != 0);
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
        REQUIRE(id.value() == 0);
    }
}

TEST_CASE("AssetId comparison operators", "[assets][assetid]") {
    limbo::AssetId id1(100);
    limbo::AssetId id2(200);
    limbo::AssetId id3(100);

    SECTION("Equality") {
        REQUIRE(id1 == id3);
        REQUIRE(!(id1 == id2));
    }

    SECTION("Inequality") {
        REQUIRE(id1 != id2);
        REQUIRE(!(id1 != id3));
    }

    SECTION("Less than") {
        REQUIRE(id1 < id2);
        REQUIRE(!(id2 < id1));
        REQUIRE(!(id1 < id3));
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
