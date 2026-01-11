#include "limbo/render/2d/SpriteAtlas.hpp"

#include "limbo/core/Assert.hpp"
#include "limbo/debug/Log.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace limbo {

void SpriteAtlas::addRegion(const SpriteRegion& region) {
    usize index = m_regions.size();
    m_regions.push_back(region);
    m_nameToIndex[region.name] = index;
}

const SpriteRegion* SpriteAtlas::getRegion(const String& name) const {
    auto it = m_nameToIndex.find(name);
    if (it != m_nameToIndex.end()) {
        return &m_regions[it->second];
    }
    return nullptr;
}

const SpriteRegion& SpriteAtlas::getRegionByIndex(usize index) const {
    LIMBO_ASSERT(index < m_regions.size(), "Region index out of bounds");
    return m_regions[index];
}

bool SpriteAtlas::hasRegion(const String& name) const {
    return m_nameToIndex.find(name) != m_nameToIndex.end();
}

std::vector<String> SpriteAtlas::getRegionNames() const {
    std::vector<String> names;
    names.reserve(m_regions.size());
    for (const auto& region : m_regions) {
        names.push_back(region.name);
    }
    return names;
}

void SpriteAtlas::clearRegions() {
    m_regions.clear();
    m_nameToIndex.clear();
}

bool SpriteAtlas::saveMetadata(const std::filesystem::path& path, const String& texturePath) const {
    try {
        nlohmann::json json;
        json["version"] = 1;
        json["texture"] = texturePath;
        json["width"] = m_width;
        json["height"] = m_height;

        nlohmann::json regionsJson = nlohmann::json::array();
        for (const auto& region : m_regions) {
            nlohmann::json regionJson;
            regionJson["name"] = region.name;
            regionJson["x"] = region.x;
            regionJson["y"] = region.y;
            regionJson["width"] = region.width;
            regionJson["height"] = region.height;
            regionJson["uvMin"] = {region.uvMin.x, region.uvMin.y};
            regionJson["uvMax"] = {region.uvMax.x, region.uvMax.y};
            regionJson["pivot"] = {region.pivot.x, region.pivot.y};
            regionJson["rotated"] = region.rotated;
            if (!region.sourceFile.empty()) {
                regionJson["sourceFile"] = region.sourceFile;
            }
            regionsJson.push_back(regionJson);
        }
        json["regions"] = regionsJson;

        std::ofstream file(path);
        if (!file.is_open()) {
            LIMBO_LOG_RENDER_ERROR("SpriteAtlas: Failed to open file for writing: {}",
                                   path.string());
            return false;
        }

        file << json.dump(2);
        return true;

    } catch (const std::exception& e) {
        LIMBO_LOG_RENDER_ERROR("SpriteAtlas: Failed to save metadata: {}", e.what());
        return false;
    }
}

String SpriteAtlas::loadMetadata(const std::filesystem::path& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LIMBO_LOG_RENDER_ERROR("SpriteAtlas: Failed to open file: {}", path.string());
            return "";
        }

        nlohmann::json json;
        file >> json;

        // Check version
        i32 version = json.value("version", 1);
        if (version != 1) {
            LIMBO_LOG_RENDER_WARN("SpriteAtlas: Unknown version {}, attempting to load anyway",
                                  version);
        }

        // Read atlas info
        m_width = json.value("width", 0u);
        m_height = json.value("height", 0u);
        String texturePath = json.value("texture", "");

        // Clear existing regions
        clearRegions();

        // Read regions
        if (json.contains("regions")) {
            for (const auto& regionJson : json["regions"]) {
                SpriteRegion region;
                region.name = regionJson.value("name", "");
                region.x = regionJson.value("x", 0u);
                region.y = regionJson.value("y", 0u);
                region.width = regionJson.value("width", 0u);
                region.height = regionJson.value("height", 0u);
                region.rotated = regionJson.value("rotated", false);

                if (regionJson.contains("uvMin")) {
                    region.uvMin.x = regionJson["uvMin"][0];
                    region.uvMin.y = regionJson["uvMin"][1];
                }
                if (regionJson.contains("uvMax")) {
                    region.uvMax.x = regionJson["uvMax"][0];
                    region.uvMax.y = regionJson["uvMax"][1];
                }
                if (regionJson.contains("pivot")) {
                    region.pivot.x = regionJson["pivot"][0];
                    region.pivot.y = regionJson["pivot"][1];
                }
                region.sourceFile = regionJson.value("sourceFile", "");

                addRegion(region);
            }
        }

        return texturePath;

    } catch (const std::exception& e) {
        LIMBO_LOG_RENDER_ERROR("SpriteAtlas: Failed to load metadata: {}", e.what());
        return "";
    }
}

}  // namespace limbo
