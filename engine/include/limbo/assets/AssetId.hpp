#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/core/UUID.hpp"

#include <functional>

namespace limbo {

/**
 * AssetId - Unique identifier for assets
 *
 * Assets are identified by a stable UUID that survives renames and moves.
 * The AssetRegistry maintains the mapping between UUIDs and file paths.
 *
 * For backwards compatibility and convenience, AssetIds can also be created
 * from paths (which generates a hash-based ID), but UUID-based IDs are preferred.
 */
class LIMBO_API AssetId {
public:
    constexpr AssetId() = default;

    /**
     * Create an AssetId from a UUID (preferred)
     */
    constexpr explicit AssetId(const UUID& uuid) : m_uuid(uuid) {}

    /**
     * Create an AssetId from a path (legacy, generates hash-based ID)
     * @deprecated Prefer UUID-based IDs for stable references
     */
    explicit AssetId(StringView path);

    /**
     * Create an AssetId from raw high/low values
     */
    constexpr AssetId(u64 high, u64 low) : m_uuid(high, low) {}

    /**
     * Get the underlying UUID
     */
    [[nodiscard]] constexpr const UUID& uuid() const { return m_uuid; }

    /**
     * Get a hash value for this ID (for use in containers)
     */
    [[nodiscard]] constexpr u64 hash() const { return m_uuid.hash(); }

    /**
     * Check if this is a valid (non-null) ID
     */
    [[nodiscard]] constexpr bool isValid() const { return m_uuid.isValid(); }

    /**
     * Convert to string representation
     */
    [[nodiscard]] String toString() const { return m_uuid.toString(); }

    // Comparison operators
    constexpr bool operator==(const AssetId& other) const { return m_uuid == other.m_uuid; }
    constexpr bool operator!=(const AssetId& other) const { return m_uuid != other.m_uuid; }
    constexpr bool operator<(const AssetId& other) const { return m_uuid < other.m_uuid; }

    /**
     * Get an invalid/null AssetId
     */
    [[nodiscard]] static constexpr AssetId invalid() { return AssetId{}; }

    /**
     * Generate a new random AssetId
     */
    [[nodiscard]] static AssetId generate() { return AssetId{UUID::generate()}; }

    /**
     * Create an AssetId from a string representation
     */
    [[nodiscard]] static AssetId fromString(StringView str) {
        return AssetId{UUID::fromString(str)};
    }

private:
    UUID m_uuid;
};

}  // namespace limbo

// Hash support for use in containers
template <>
struct std::hash<limbo::AssetId> {
    std::size_t operator()(const limbo::AssetId& id) const noexcept {
        return static_cast<std::size_t>(id.hash());
    }
};
