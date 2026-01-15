#include <catch2/catch_test_macros.hpp>

#include <limbo/scripting/ScriptEngine.hpp>
#include <limbo/scripting/ScriptComponent.hpp>
#include <limbo/ecs/World.hpp>
#include <limbo/ecs/Entity.hpp>
#include <limbo/ecs/Components.hpp>

#include <filesystem>
#include <fstream>
#include <regex>

namespace {

// Helper to create a temporary script file
class TempScript {
public:
    explicit TempScript(const std::string& content) {
        m_path = std::filesystem::temp_directory_path() /
                 ("test_script_" + std::to_string(s_counter++) + ".lua");
        std::ofstream file(m_path);
        file << content;
    }

    ~TempScript() {
        if (std::filesystem::exists(m_path)) {
            std::filesystem::remove(m_path);
        }
    }

    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
    static inline int s_counter = 0;
};

}  // namespace

TEST_CASE("ScriptEngine initialization", "[scripting]") {
    limbo::ScriptEngine engine;

    SECTION("Engine initializes successfully") {
        REQUIRE(engine.init());
    }

    SECTION("Lua state is valid after init") {
        REQUIRE(engine.init());
        auto& lua = engine.getLuaState();
        REQUIRE(lua.lua_state() != nullptr);
    }

    SECTION("Can execute simple Lua code") {
        REQUIRE(engine.init());
        auto& lua = engine.getLuaState();

        auto result = lua.safe_script("x = 1 + 1", sol::script_pass_on_error);
        REQUIRE(result.valid());

        int x = lua["x"];
        REQUIRE(x == 2);
    }
}

TEST_CASE("ScriptEngine global tables", "[scripting]") {
    limbo::ScriptEngine engine;
    REQUIRE(engine.init());
    auto& lua = engine.getLuaState();

    SECTION("Input table exists") {
        sol::object input = lua["Input"];
        REQUIRE(input.valid());
        REQUIRE(input.get_type() == sol::type::table);
    }

    SECTION("Time table exists") {
        sol::object time = lua["Time"];
        REQUIRE(time.valid());
        REQUIRE(time.get_type() == sol::type::table);
    }

    SECTION("log table exists") {
        sol::object log = lua["log"];
        REQUIRE(log.valid());
        REQUIRE(log.get_type() == sol::type::table);
    }
}

TEST_CASE("ScriptEngine math types", "[scripting]") {
    limbo::ScriptEngine engine;
    REQUIRE(engine.init());
    auto& lua = engine.getLuaState();

    SECTION("Vec2 operations") {
        auto result = lua.safe_script(R"(
            local v1 = Vec2(3, 4)
            x_val = v1.x
            y_val = v1.y
            len = v1:length()
        )",
                                      sol::script_pass_on_error);
        REQUIRE(result.valid());

        float x = lua["x_val"];
        float y = lua["y_val"];
        float len = lua["len"];

        REQUIRE(x == 3.0f);
        REQUIRE(y == 4.0f);
        REQUIRE(len == 5.0f);  // 3-4-5 triangle
    }

    SECTION("Vec2 arithmetic") {
        auto result = lua.safe_script(R"(
            local v1 = Vec2(1, 2)
            local v2 = Vec2(3, 4)
            local sum = v1 + v2
            sum_x = sum.x
            sum_y = sum.y
        )",
                                      sol::script_pass_on_error);
        REQUIRE(result.valid());

        float x = lua["sum_x"];
        float y = lua["sum_y"];

        REQUIRE(x == 4.0f);
        REQUIRE(y == 6.0f);
    }

    SECTION("Vec3 operations") {
        auto result = lua.safe_script(R"(
            local v = Vec3(1, 2, 3)
            x_val = v.x
            y_val = v.y
            z_val = v.z
        )",
                                      sol::script_pass_on_error);
        REQUIRE(result.valid());

        float x = lua["x_val"];
        float y = lua["y_val"];
        float z = lua["z_val"];

        REQUIRE(x == 1.0f);
        REQUIRE(y == 2.0f);
        REQUIRE(z == 3.0f);
    }

    SECTION("Vec4/Color operations") {
        auto result = lua.safe_script(R"(
            local c = Vec4(0.5, 0.6, 0.7, 1.0)
            r_val = c.r
            g_val = c.g
            b_val = c.b
            a_val = c.a
        )",
                                      sol::script_pass_on_error);
        REQUIRE(result.valid());

        float r = lua["r_val"];
        float g = lua["g_val"];
        float b = lua["b_val"];
        float a = lua["a_val"];

        REQUIRE(r == 0.5f);
        REQUIRE(g == 0.6f);
        REQUIRE(b == 0.7f);
        REQUIRE(a == 1.0f);
    }
}

TEST_CASE("ScriptEngine World binding", "[scripting]") {
    limbo::ScriptEngine engine;
    limbo::World world;

    REQUIRE(engine.init());
    engine.bindWorld(&world);

    auto& lua = engine.getLuaState();

    SECTION("World table exists after binding") {
        sol::table worldTable = lua["World"];
        REQUIRE(worldTable.valid());
    }

    SECTION("Can create entity from Lua") {
        auto result = lua.safe_script(R"(
            entity = World.createEntity("LuaEntity")
            entity_valid = entity:isValid()
            entity_name = entity:getName()
        )",
                                      sol::script_pass_on_error);
        REQUIRE(result.valid());

        bool valid = lua["entity_valid"];
        std::string name = lua["entity_name"];

        REQUIRE(valid);
        REQUIRE(name == "LuaEntity");
        REQUIRE(world.entityCount() == 1);
    }

    SECTION("Can get entity by name") {
        // Create entity through C++
        world.createEntity("TestEntity");

        auto result = lua.safe_script(R"(
            found = World.getEntityByName("TestEntity")
            found_valid = found:isValid()
        )",
                                      sol::script_pass_on_error);
        REQUIRE(result.valid());

        bool valid = lua["found_valid"];
        REQUIRE(valid);
    }
}

TEST_CASE("ScriptComponent", "[scripting]") {
    SECTION("Default construction") {
        limbo::ScriptComponent script;
        REQUIRE(script.scriptPath.empty());
        REQUIRE_FALSE(script.initialized);
        REQUIRE_FALSE(script.started);
        REQUIRE(script.enabled);
        REQUIRE_FALSE(script.hasError());
    }

    SECTION("Path construction") {
        limbo::ScriptComponent script("test/path.lua");
        REQUIRE(script.scriptPath == "test/path.lua");
    }

    SECTION("Error handling") {
        limbo::ScriptComponent script;
        script.lastError = "Test error";
        script.lastErrorLine = 42;

        REQUIRE(script.hasError());

        script.clearError();
        REQUIRE_FALSE(script.hasError());
        REQUIRE(script.lastError.empty());
        REQUIRE(script.lastErrorLine == 0);
    }
}

TEST_CASE("Script loading and execution", "[scripting]") {
    limbo::ScriptEngine engine;
    limbo::World world;

    REQUIRE(engine.init());
    engine.bindWorld(&world);

    auto& lua = engine.getLuaState();

    SECTION("Load and execute simple script") {
        TempScript script(R"(
            test_value = 42
            function onStart()
                test_started = true
            end
        )");

        // Create environment and load script
        sol::environment env(lua, sol::create, lua.globals());
        auto result = lua.safe_script_file(script.path().string(), env, sol::script_pass_on_error);
        REQUIRE(result.valid());

        int value = env["test_value"];
        REQUIRE(value == 42);

        // Call onStart
        sol::protected_function onStart = env["onStart"];
        REQUIRE(onStart.valid());

        auto callResult = onStart();
        REQUIRE(callResult.valid());

        bool started = env["test_started"];
        REQUIRE(started);
    }

    SECTION("Script with syntax error reports error") {
        TempScript script(R"(
            this is not valid lua syntax!!!
        )");

        sol::environment env(lua, sol::create, lua.globals());
        auto result = lua.safe_script_file(script.path().string(), env, sol::script_pass_on_error);
        REQUIRE_FALSE(result.valid());
    }

    SECTION("Script can access Vec2") {
        TempScript script(R"(
            local pos = Vec2(10, 20)
            result_x = pos.x
            result_y = pos.y
        )");

        sol::environment env(lua, sol::create, lua.globals());
        auto result = lua.safe_script_file(script.path().string(), env, sol::script_pass_on_error);
        REQUIRE(result.valid());

        float x = env["result_x"];
        float y = env["result_y"];
        REQUIRE(x == 10.0f);
        REQUIRE(y == 20.0f);
    }
}

TEST_CASE("Entity API from scripts", "[scripting]") {
    limbo::ScriptEngine engine;
    limbo::World world;

    REQUIRE(engine.init());
    engine.bindWorld(&world);

    auto& lua = engine.getLuaState();

    // Create an entity with transform
    auto entity = world.createEntity("TestEntity");
    world.addComponent<limbo::TransformComponent>(entity.id(), glm::vec3(5.0f, 10.0f, 0.0f));

    // Create environment with 'self' reference
    sol::environment env(lua, sol::create, lua.globals());
    env["self"] = limbo::Entity(entity.id(), &world);

    SECTION("Get entity position") {
        auto result = lua.safe_script(R"(
            local pos = self:getPosition()
            pos_x = pos.x
            pos_y = pos.y
        )",
                                      env, sol::script_pass_on_error);
        REQUIRE(result.valid());

        float x = env["pos_x"];
        float y = env["pos_y"];
        REQUIRE(x == 5.0f);
        REQUIRE(y == 10.0f);
    }

    SECTION("Set entity position") {
        auto result = lua.safe_script(R"(
            self:setPosition(Vec3(100, 200, 0))
        )",
                                      env, sol::script_pass_on_error);

        std::string errorMsg;
        if (!result.valid()) {
            sol::error err = result;
            errorMsg = err.what();
        }
        INFO("Lua error: " << errorMsg);
        REQUIRE(result.valid());

        auto& transform = world.getComponent<limbo::TransformComponent>(entity.id());
        REQUIRE(transform.position.x == 100.0f);
        REQUIRE(transform.position.y == 200.0f);
    }

    SECTION("Get entity name") {
        auto result = lua.safe_script(R"(
            name = self:getName()
        )",
                                      env, sol::script_pass_on_error);
        REQUIRE(result.valid());

        std::string name = env["name"];
        REQUIRE(name == "TestEntity");
    }

    SECTION("Check entity validity") {
        auto result = lua.safe_script(R"(
            valid = self:isValid()
        )",
                                      env, sol::script_pass_on_error);
        REQUIRE(result.valid());

        bool valid = env["valid"];
        REQUIRE(valid);
    }
}

TEST_CASE("Script error parsing", "[scripting]") {
    // Test the error message format that Sol2 produces
    SECTION("Can identify error patterns") {
        // Sol2 format: [string "path"]:line: message
        std::string error1 = R"([string "assets/scripts/test.lua"]:15: attempt to index nil value)";

        // Check that patterns work
        std::regex pattern(R"(\[string \"([^\"]+)\"\]:(\d+):\s*(.*))");
        std::smatch match;

        REQUIRE(std::regex_search(error1, match, pattern));
        REQUIRE(match[1].str() == "assets/scripts/test.lua");
        REQUIRE(match[2].str() == "15");
        REQUIRE(match[3].str() == "attempt to index nil value");
    }
}
