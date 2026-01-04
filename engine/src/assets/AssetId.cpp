#include "limbo/assets/AssetId.hpp"

#include <functional>

namespace limbo {

AssetId::AssetId(StringView path) {
    // Use FNV-1a hash for asset IDs
    constexpr u64 kFnvOffset = 14695981039346656037ULL;
    constexpr u64 kFnvPrime = 1099511628211ULL;

    u64 hash = kFnvOffset;
    for (char const c : path) {
        hash ^= static_cast<u64>(c);
        hash *= kFnvPrime;
    }
    m_id = hash;
}

}  // namespace limbo
