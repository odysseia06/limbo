#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <variant>

// std::expected is C++23 - check for actual availability, not just header presence
// MSVC: _MSVC_LANG >= 202302L for C++23
// GCC/Clang: __cplusplus >= 202302L for C++23
#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#include <expected>
#define LIMBO_HAS_EXPECTED 1
#else
#define LIMBO_HAS_EXPECTED 0
#endif

namespace limbo {

// Fixed-size integer types
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Floating point types
using f32 = float;
using f64 = double;

// Size types
using usize = std::size_t;
using isize = std::ptrdiff_t;

// Smart pointer aliases
template <typename T>
using Unique = std::unique_ptr<T>;

template <typename T>
using Shared = std::shared_ptr<T>;

template <typename T>
using Weak = std::weak_ptr<T>;

// Helper functions for smart pointers
template <typename T, typename... Args>
constexpr Unique<T> make_unique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
constexpr Shared<T> make_shared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// String types
using String = std::string;
using StringView = std::string_view;

// Optional type alias
template <typename T>
using Optional = std::optional<T>;

// Forward declaration for unexpected
template <typename E>
struct unexpected;

// Result type for fallible operations
#if LIMBO_HAS_EXPECTED
template <typename T, typename E = String>
using Result = std::expected<T, E>;

using std::unexpected;
#else
// unexpected wrapper for error values
template <typename E>
struct unexpected {
    E m_error;
    explicit unexpected(const E& e) : m_error(e) {}
    explicit unexpected(E&& e) : m_error(std::move(e)) {}
    [[nodiscard]] const E& error() const { return m_error; }
};

// Fallback Result implementation for compilers without std::expected
template <typename T, typename E = String>
class Result {
public:
    Result(const T& value) : m_value(value), m_hasValue(true) {}
    Result(T&& value) : m_value(std::move(value)), m_hasValue(true) {}

    Result(const unexpected<E>& err) : m_error(err.error()) {}
    Result(unexpected<E>&& err) : m_error(std::move(err.m_error)) {}

    [[nodiscard]] bool has_value() const { return m_hasValue; }
    explicit operator bool() const { return m_hasValue; }

    T& value() { return m_value; }
    [[nodiscard]] const T& value() const { return m_value; }
    T& operator*() { return m_value; }
    const T& operator*() const { return m_value; }

    E& error() { return m_error; }
    [[nodiscard]] const E& error() const { return m_error; }

private:
    T m_value{};
    E m_error{};
    bool m_hasValue = false;
};

// Specialization for void
template <typename E>
class Result<void, E> {
public:
    Result() : m_hasValue(true) {}

    Result(const unexpected<E>& err) : m_error(err.error()) {}
    Result(unexpected<E>&& err) : m_error(std::move(err.m_error)) {}

    [[nodiscard]] bool has_value() const { return m_hasValue; }
    explicit operator bool() const { return m_hasValue; }

    E& error() { return m_error; }
    [[nodiscard]] const E& error() const { return m_error; }

private:
    E m_error{};
    bool m_hasValue = false;
};
#endif

}  // namespace limbo
