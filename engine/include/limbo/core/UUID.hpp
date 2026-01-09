#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <functional>
#include <string>

namespace limbo {

/**
 * UUID - A universally unique identifier (128-bit)
 *
 * Used for stable asset identification that survives renames and moves.
 * Implements UUID v4 (random) generation.
 */
class LIMBO_API UUID {
public:
    /**
     * Create a null UUID (all zeros)
     */
    constexpr UUID() = default;

    /**
     * Create a UUID from two 64-bit values
     */
    constexpr UUID(u64 high, u64 low) : m_high(high), m_low(low) {}

    /**
     * Generate a new random UUID (v4)
     */
    [[nodiscard]] static UUID generate();

    /**
     * Create a UUID from a string representation
     * Accepts formats: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" or "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
     */
    [[nodiscard]] static UUID fromString(StringView str);

    /**
     * Convert to string representation "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
     */
    [[nodiscard]] String toString() const;

    /**
     * Convert to compact string representation (no dashes)
     */
    [[nodiscard]] String toCompactString() const;

    /**
     * Check if this is a null UUID
     */
    [[nodiscard]] constexpr bool isNull() const { return m_high == 0 && m_low == 0; }

    /**
     * Check if this is a valid (non-null) UUID
     */
    [[nodiscard]] constexpr bool isValid() const { return !isNull(); }

    /**
     * Get the high 64 bits
     */
    [[nodiscard]] constexpr u64 high() const { return m_high; }

    /**
     * Get the low 64 bits
     */
    [[nodiscard]] constexpr u64 low() const { return m_low; }

    /**
     * Get a hash value for use in containers
     */
    [[nodiscard]] constexpr u64 hash() const {
        // Simple hash combining high and low parts
        return m_high ^ (m_low * 0x9e3779b97f4a7c15ULL);
    }

    // Comparison operators
    constexpr bool operator==(const UUID& other) const {
        return m_high == other.m_high && m_low == other.m_low;
    }

    constexpr bool operator!=(const UUID& other) const { return !(*this == other); }

    constexpr bool operator<(const UUID& other) const {
        if (m_high != other.m_high) {
            return m_high < other.m_high;
        }
        return m_low < other.m_low;
    }

    /**
     * Get a null UUID
     */
    [[nodiscard]] static constexpr UUID null() { return UUID{}; }

private:
    u64 m_high = 0;
    u64 m_low = 0;
};

}  // namespace limbo

// Hash support for use in containers
template <>
struct std::hash<limbo::UUID> {
    std::size_t operator()(const limbo::UUID& uuid) const noexcept {
        return static_cast<std::size_t>(uuid.hash());
    }
};
