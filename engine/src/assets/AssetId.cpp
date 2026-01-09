#include "limbo/assets/AssetId.hpp"

namespace limbo {

AssetId::AssetId(StringView path) {
    // Use FNV-1a hash for path-based asset IDs (legacy compatibility)
    // This generates a deterministic ID from the path, but is NOT stable
    // across renames/moves. Prefer UUID-based IDs for new assets.
    constexpr u64 kFnvOffset = 14695981039346656037ULL;
    constexpr u64 kFnvPrime = 1099511628211ULL;

    u64 high = kFnvOffset;
    for (char const c : path) {
        high ^= static_cast<u64>(c);
        high *= kFnvPrime;
    }

    // Generate a second hash for the low part
    u64 low = kFnvOffset ^ 0x12345678ABCDEF00ULL;
    for (char const c : path) {
        low ^= static_cast<u64>(c);
        low *= kFnvPrime;
    }

    m_uuid = UUID{high, low};
}

}  // namespace limbo
