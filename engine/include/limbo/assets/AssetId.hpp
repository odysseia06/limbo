#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <functional>

namespace limbo {

class LIMBO_API AssetId {
public:
    constexpr AssetId() = default;
    constexpr explicit AssetId(u64 id) : m_id(id) {}
    explicit AssetId(StringView path);

    [[nodiscard]] constexpr u64 value() const { return m_id; }
    [[nodiscard]] constexpr bool isValid() const { return m_id != 0; }

    constexpr bool operator==(const AssetId& other) const { return m_id == other.m_id; }
    constexpr bool operator!=(const AssetId& other) const { return m_id != other.m_id; }
    constexpr bool operator<(const AssetId& other) const { return m_id < other.m_id; }

    [[nodiscard]] static constexpr AssetId invalid() { return AssetId{0}; }

private:
    u64 m_id = 0;
};

}  // namespace limbo

// Hash support for use in containers
template <>
struct std::hash<limbo::AssetId> {
    std::size_t operator()(const limbo::AssetId& id) const noexcept {
        return std::hash<limbo::u64>{}(id.value());
    }
};
