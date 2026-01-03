#include <catch2/catch_test_macros.hpp>

#include <limbo/core/Types.hpp>

TEST_CASE("Type aliases have correct sizes", "[core][types]") {
    SECTION("Integer types") {
        REQUIRE(sizeof(limbo::i8) == 1);
        REQUIRE(sizeof(limbo::i16) == 2);
        REQUIRE(sizeof(limbo::i32) == 4);
        REQUIRE(sizeof(limbo::i64) == 8);

        REQUIRE(sizeof(limbo::u8) == 1);
        REQUIRE(sizeof(limbo::u16) == 2);
        REQUIRE(sizeof(limbo::u32) == 4);
        REQUIRE(sizeof(limbo::u64) == 8);
    }

    SECTION("Floating point types") {
        REQUIRE(sizeof(limbo::f32) == 4);
        REQUIRE(sizeof(limbo::f64) == 8);
    }
}

TEST_CASE("Smart pointer helpers work correctly", "[core][types]") {
    SECTION("make_unique creates unique pointer") {
        auto ptr = limbo::make_unique<int>(42);
        REQUIRE(ptr != nullptr);
        REQUIRE(*ptr == 42);
    }

    SECTION("make_shared creates shared pointer") {
        auto ptr = limbo::make_shared<int>(42);
        REQUIRE(ptr != nullptr);
        REQUIRE(*ptr == 42);
        REQUIRE(ptr.use_count() == 1);
    }
}

TEST_CASE("Result type works correctly", "[core][types]") {
    SECTION("Success case") {
        limbo::Result<int> result = 42;
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 42);
    }

    SECTION("Error case") {
        limbo::Result<int> result = limbo::unexpected<limbo::String>("error");
        REQUIRE(!result.has_value());
        REQUIRE(result.error() == "error");
    }
}
