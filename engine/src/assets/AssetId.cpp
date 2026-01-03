#include "limbo/assets/AssetId.hpp"

#include <functional>

namespace limbo {

AssetId::AssetId(StringView path) {
    // Use FNV-1a hash for asset IDs
    constexpr u64 FNV_OFFSET = 14695981039346656037ULL;
    constexpr u64 FNV_PRIME = 1099511628211ULL;

    u64 hash = FNV_OFFSET;
    for (char c : path) {
        hash ^= static_cast<u64>(c);
        hash *= FNV_PRIME;
    }
    m_id = hash;
}

}  // namespace limbo
