#include <catch2/catch_test_macros.hpp>

#include "limbo/assets/HotReloadManager.hpp"

using namespace limbo;

TEST_CASE("HotReloadManager dependency tracking", "[assets][hotreload]") {
    HotReloadManager manager;

    // Create some asset IDs
    AssetId textureA = AssetId::generate();
    AssetId textureB = AssetId::generate();
    AssetId materialA = AssetId::generate();
    AssetId materialB = AssetId::generate();

    SECTION("can add and query dependencies") {
        // materialA depends on textureA
        manager.addDependency(materialA, textureA);

        std::vector<AssetId> deps = manager.getDependencies(materialA);
        REQUIRE(deps.size() == 1);
        REQUIRE(deps[0] == textureA);

        std::vector<AssetId> dependents = manager.getDependents(textureA);
        REQUIRE(dependents.size() == 1);
        REQUIRE(dependents[0] == materialA);
    }

    SECTION("can remove dependencies") {
        manager.addDependency(materialA, textureA);
        manager.addDependency(materialA, textureB);

        REQUIRE(manager.getDependencies(materialA).size() == 2);

        manager.removeDependency(materialA, textureA);

        std::vector<AssetId> deps = manager.getDependencies(materialA);
        REQUIRE(deps.size() == 1);
        REQUIRE(deps[0] == textureB);
    }

    SECTION("can clear all dependencies for an asset") {
        manager.addDependency(materialA, textureA);
        manager.addDependency(materialA, textureB);

        REQUIRE(manager.getDependencies(materialA).size() == 2);

        manager.clearDependencies(materialA);

        REQUIRE(manager.getDependencies(materialA).empty());
        REQUIRE(manager.getDependents(textureA).empty());
        REQUIRE(manager.getDependents(textureB).empty());
    }

    SECTION("getAffectedAssets returns transitive dependents") {
        // Build dependency chain: textureA <- materialA <- compositeA
        AssetId compositeA = AssetId::generate();

        manager.addDependency(materialA, textureA);
        manager.addDependency(compositeA, materialA);

        std::vector<AssetId> affected = manager.getAffectedAssets(textureA);

        // Should include textureA, materialA, and compositeA
        REQUIRE(affected.size() == 3);

        bool hasTexture = false;
        bool hasMaterial = false;
        bool hasComposite = false;
        for (AssetId id : affected) {
            if (id == textureA)
                hasTexture = true;
            if (id == materialA)
                hasMaterial = true;
            if (id == compositeA)
                hasComposite = true;
        }

        REQUIRE(hasTexture);
        REQUIRE(hasMaterial);
        REQUIRE(hasComposite);
    }

    SECTION("handles diamond dependencies") {
        // textureA is used by both materialA and materialB
        // Both materials are used by compositeA
        AssetId compositeA = AssetId::generate();

        manager.addDependency(materialA, textureA);
        manager.addDependency(materialB, textureA);
        manager.addDependency(compositeA, materialA);
        manager.addDependency(compositeA, materialB);

        std::vector<AssetId> affected = manager.getAffectedAssets(textureA);

        // Should include all 4 assets, but no duplicates
        REQUIRE(affected.size() == 4);
    }
}

TEST_CASE("HotReloadManager reload triggering", "[assets][hotreload]") {
    HotReloadManager manager;
    manager.setEnabled(true);
    manager.setBatchReloads(true);

    AssetId textureA = AssetId::generate();
    AssetId materialA = AssetId::generate();

    // Track reloads
    std::vector<AssetId> reloadedAssets;
    manager.setReloadHandler([&reloadedAssets](AssetId id) {
        reloadedAssets.push_back(id);
        return true;
    });

    manager.addDependency(materialA, textureA);

    SECTION("triggerReload schedules asset and dependents") {
        manager.triggerReload(textureA);

        // Should have 2 pending reloads
        REQUIRE(manager.getPendingReloadCount() == 2);
    }

    SECTION("processPendingReloads calls reload handler") {
        manager.triggerReload(textureA);
        manager.processPendingReloads();

        // Both assets should have been reloaded
        REQUIRE(reloadedAssets.size() == 2);

        // Pending should be cleared
        REQUIRE(manager.getPendingReloadCount() == 0);
    }

    SECTION("reloads are in dependency order") {
        // textureA should be reloaded before materialA
        manager.triggerReload(textureA);
        manager.processPendingReloads();

        REQUIRE(reloadedAssets.size() == 2);

        // Find indices
        usize textureIndex = 0;
        usize materialIndex = 0;
        for (usize i = 0; i < reloadedAssets.size(); i++) {
            if (reloadedAssets[i] == textureA)
                textureIndex = i;
            if (reloadedAssets[i] == materialA)
                materialIndex = i;
        }

        // textureA should come before materialA
        REQUIRE(textureIndex < materialIndex);
    }
}

TEST_CASE("HotReloadManager callbacks", "[assets][hotreload]") {
    HotReloadManager manager;
    manager.setEnabled(true);
    manager.setBatchReloads(false);  // Process immediately

    AssetId assetA = AssetId::generate();

    SECTION("before-reload callback can cancel reload") {
        bool reloadCalled = false;
        manager.setReloadHandler([&reloadCalled](AssetId) {
            reloadCalled = true;
            return true;
        });

        manager.setBeforeReloadCallback([](AssetId) {
            return false;  // Cancel
        });

        manager.triggerReload(assetA);

        REQUIRE_FALSE(reloadCalled);
    }

    SECTION("after-reload callback receives event") {
        ReloadEvent receivedEvent;
        bool callbackCalled = false;

        manager.setReloadHandler([](AssetId) { return true; });

        manager.setAfterReloadCallback([&](const ReloadEvent& event) {
            receivedEvent = event;
            callbackCalled = true;
        });

        manager.triggerReload(assetA);

        REQUIRE(callbackCalled);
        REQUIRE(receivedEvent.assetId == assetA);
        REQUIRE(receivedEvent.success);
    }

    SECTION("after-reload callback reports failure") {
        ReloadEvent receivedEvent;

        manager.setReloadHandler([](AssetId) { return false; });

        manager.setAfterReloadCallback([&](const ReloadEvent& event) { receivedEvent = event; });

        manager.triggerReload(assetA);

        REQUIRE_FALSE(receivedEvent.success);
    }
}

TEST_CASE("HotReloadManager statistics", "[assets][hotreload]") {
    HotReloadManager manager;
    manager.setEnabled(true);
    manager.setBatchReloads(false);

    AssetId assetA = AssetId::generate();
    AssetId assetB = AssetId::generate();

    SECTION("tracks total reloads") {
        manager.setReloadHandler([](AssetId) { return true; });

        REQUIRE(manager.getTotalReloads() == 0);

        manager.triggerReload(assetA);
        REQUIRE(manager.getTotalReloads() == 1);

        manager.triggerReload(assetB);
        REQUIRE(manager.getTotalReloads() == 2);
    }

    SECTION("tracks failed reloads") {
        manager.setReloadHandler([](AssetId) { return false; });

        REQUIRE(manager.getFailedReloads() == 0);

        manager.triggerReload(assetA);
        REQUIRE(manager.getFailedReloads() == 1);
    }

    SECTION("can reset statistics") {
        manager.setReloadHandler([](AssetId) { return true; });

        manager.triggerReload(assetA);
        REQUIRE(manager.getTotalReloads() == 1);

        manager.resetStats();
        REQUIRE(manager.getTotalReloads() == 0);
        REQUIRE(manager.getFailedReloads() == 0);
    }
}

TEST_CASE("HotReloadManager configuration", "[assets][hotreload]") {
    HotReloadManager manager;

    SECTION("starts disabled") {
        REQUIRE_FALSE(manager.isEnabled());
    }

    SECTION("can enable/disable") {
        manager.setEnabled(true);
        REQUIRE(manager.isEnabled());

        manager.setEnabled(false);
        REQUIRE_FALSE(manager.isEnabled());
    }
}
