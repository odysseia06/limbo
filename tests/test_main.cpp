#include <catch2/catch_session.hpp>

#include <limbo/debug/Log.hpp>

#include <cstring>

int main(int argc, char* argv[]) {
    // Check if we're just listing tests (used by CTest discovery)
    // In that case, skip logging to avoid polluting stdout
    bool isListing = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--list-tests") == 0 ||
            std::strcmp(argv[i], "--list-test-names-only") == 0 ||
            std::strcmp(argv[i], "--list-tags") == 0) {
            isListing = true;
            break;
        }
    }

    if (!isListing) {
        // Initialize logging before running tests
        limbo::log::init();
    }

    // Run Catch2 tests
    int result = Catch::Session().run(argc, argv);

    if (!isListing) {
        // Shutdown logging
        limbo::log::shutdown();
    }

    return result;
}
