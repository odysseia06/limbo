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

TEST_CASE("HotReloadManager shared dependency paths", "[assets][hotreload]") {
    HotReloadManager manager;
    manager.setEnabled(true);
    manager.setBatchReloads(false);

    // Simulate multiple assets sharing the same source file
    // (e.g., multiple sprite regions from same texture atlas)
    AssetId atlasA = AssetId::generate();
    AssetId atlasB = AssetId::generate();
    AssetId spriteA = AssetId::generate();
    AssetId spriteB = AssetId::generate();

    std::vector<AssetId> reloadedAssets;
    manager.setReloadHandler([&reloadedAssets](AssetId id) {
        reloadedAssets.push_back(id);
        return true;
    });

    SECTION("multiple assets can watch same path") {
        // Both atlases watch the same texture file
        std::filesystem::path sharedPath = "textures/shared_atlas.png";

        manager.watchAsset(atlasA, sharedPath);
        manager.watchAsset(atlasB, sharedPath);

        REQUIRE(manager.isWatching(atlasA));
        REQUIRE(manager.isWatching(atlasB));
    }

    SECTION("both assets reload when shared path changes") {
        std::filesystem::path sharedPath = "textures/shared_atlas.png";

        manager.watchAsset(atlasA, sharedPath);
        manager.watchAsset(atlasB, sharedPath);

        // Sprites depend on their respective atlases
        manager.addDependency(spriteA, atlasA);
        manager.addDependency(spriteB, atlasB);

        // Trigger reload for atlasA (simulating file change)
        manager.triggerReload(atlasA);

        // Should reload atlasA and spriteA
        bool hasAtlasA = false;
        bool hasSpriteA = false;
        for (AssetId id : reloadedAssets) {
            if (id == atlasA)
                hasAtlasA = true;
            if (id == spriteA)
                hasSpriteA = true;
        }
        REQUIRE(hasAtlasA);
        REQUIRE(hasSpriteA);
    }

    SECTION("unwatching one asset doesn't affect others watching same path") {
        std::filesystem::path sharedPath = "textures/shared_atlas.png";

        manager.watchAsset(atlasA, sharedPath);
        manager.watchAsset(atlasB, sharedPath);

        manager.unwatchAsset(atlasA);

        REQUIRE_FALSE(manager.isWatching(atlasA));
        REQUIRE(manager.isWatching(atlasB));  // Should still be watched
    }
}

TEST_CASE("HotReloadManager unwatch behavior", "[assets][hotreload]") {
    HotReloadManager manager;
    manager.setEnabled(true);
    manager.setBatchReloads(false);

    AssetId assetA = AssetId::generate();
    AssetId assetB = AssetId::generate();
    AssetId dependentC = AssetId::generate();

    std::vector<AssetId> reloadedAssets;
    manager.setReloadHandler([&reloadedAssets](AssetId id) {
        reloadedAssets.push_back(id);
        return true;
    });

    SECTION("unwatchAsset removes file watching") {
        manager.watchAsset(assetA, "path/to/asset.png");
        REQUIRE(manager.isWatching(assetA));

        manager.unwatchAsset(assetA);
        REQUIRE_FALSE(manager.isWatching(assetA));
    }

    SECTION("unwatchAll removes all watches") {
        manager.watchAsset(assetA, "path/to/asset_a.png");
        manager.watchAsset(assetB, "path/to/asset_b.png");

        REQUIRE(manager.isWatching(assetA));
        REQUIRE(manager.isWatching(assetB));

        manager.unwatchAll();

        REQUIRE_FALSE(manager.isWatching(assetA));
        REQUIRE_FALSE(manager.isWatching(assetB));
    }

    SECTION("triggerReload on unwatched asset still works") {
        // Even if not watching file, we can still manually trigger reload
        manager.triggerReload(assetA);

        REQUIRE(reloadedAssets.size() == 1);
        REQUIRE(reloadedAssets[0] == assetA);
    }

    SECTION("unwatching asset preserves its dependencies") {
        manager.watchAsset(assetA, "path/to/asset.png");
        manager.addDependency(dependentC, assetA);

        manager.unwatchAsset(assetA);

        // Dependencies should still exist
        std::vector<AssetId> deps = manager.getDependencies(dependentC);
        REQUIRE(deps.size() == 1);
        REQUIRE(deps[0] == assetA);
    }

    SECTION("clearDependencies removes asset from dependency graph") {
        manager.addDependency(dependentC, assetA);
        manager.addDependency(dependentC, assetB);

        REQUIRE(manager.getDependencies(dependentC).size() == 2);

        manager.clearDependencies(dependentC);

        REQUIRE(manager.getDependencies(dependentC).empty());
        REQUIRE(manager.getDependents(assetA).empty());
        REQUIRE(manager.getDependents(assetB).empty());
    }
}

TEST_CASE("HotReloadManager complex dependency chains", "[assets][hotreload]") {
    HotReloadManager manager;
    manager.setEnabled(true);
    manager.setBatchReloads(true);

    // Create a complex dependency graph:
    // textureA <- materialA <- modelA
    // textureA <- materialB <- modelA
    // textureB <- materialB
    // (modelA depends on both materialA and materialB, which share textureA)

    AssetId textureA = AssetId::generate();
    AssetId textureB = AssetId::generate();
    AssetId materialA = AssetId::generate();
    AssetId materialB = AssetId::generate();
    AssetId modelA = AssetId::generate();

    manager.addDependency(materialA, textureA);
    manager.addDependency(materialB, textureA);
    manager.addDependency(materialB, textureB);
    manager.addDependency(modelA, materialA);
    manager.addDependency(modelA, materialB);

    std::vector<AssetId> reloadedAssets;
    manager.setReloadHandler([&reloadedAssets](AssetId id) {
        reloadedAssets.push_back(id);
        return true;
    });

    SECTION("changing textureA reloads entire dependent chain") {
        manager.triggerReload(textureA);
        manager.processPendingReloads();

        // Should reload: textureA, materialA, materialB, modelA
        REQUIRE(reloadedAssets.size() == 4);

        // Check all expected assets are present
        auto contains = [&](AssetId id) {
            return std::find(reloadedAssets.begin(), reloadedAssets.end(), id) !=
                   reloadedAssets.end();
        };

        REQUIRE(contains(textureA));
        REQUIRE(contains(materialA));
        REQUIRE(contains(materialB));
        REQUIRE(contains(modelA));
    }

    SECTION("changing textureB only reloads partial chain") {
        manager.triggerReload(textureB);
        manager.processPendingReloads();

        // Should reload: textureB, materialB, modelA
        REQUIRE(reloadedAssets.size() == 3);

        auto contains = [&](AssetId id) {
            return std::find(reloadedAssets.begin(), reloadedAssets.end(), id) !=
                   reloadedAssets.end();
        };

        REQUIRE(contains(textureB));
        REQUIRE(contains(materialB));
        REQUIRE(contains(modelA));
        REQUIRE_FALSE(contains(textureA));
        REQUIRE_FALSE(contains(materialA));
    }

    SECTION("reloads are deduplicated in dependency order") {
        manager.triggerReload(textureA);
        manager.processPendingReloads();

        // Find indices
        usize textureAIdx = 0;
        usize materialAIdx = 0;
        usize materialBIdx = 0;
        usize modelAIdx = 0;

        for (usize i = 0; i < reloadedAssets.size(); ++i) {
            if (reloadedAssets[i] == textureA)
                textureAIdx = i;
            if (reloadedAssets[i] == materialA)
                materialAIdx = i;
            if (reloadedAssets[i] == materialB)
                materialBIdx = i;
            if (reloadedAssets[i] == modelA)
                modelAIdx = i;
        }

        // textureA should come before materials
        REQUIRE(textureAIdx < materialAIdx);
        REQUIRE(textureAIdx < materialBIdx);

        // materials should come before model
        REQUIRE(materialAIdx < modelAIdx);
        REQUIRE(materialBIdx < modelAIdx);
    }
}
