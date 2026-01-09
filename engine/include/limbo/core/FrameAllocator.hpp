#pragma once

#include "limbo/core/Base.hpp"
#include "limbo/core/Types.hpp"

#include <cstdlib>
#include <new>

namespace limbo {

/**
 * FrameAllocator - Linear bump-pointer allocator reset each frame
 *
 * Provides extremely fast allocation for temporary data that only needs
 * to live for a single frame. Memory is NOT freed individually - call
 * reset() once per frame at the start of the frame.
 *
 * Usage:
 *   FrameAllocator allocator(1024 * 1024);  // 1MB
 *
 *   // Each frame:
 *   allocator.reset();
 *   auto* data = allocator.allocateArray<Vertex>(1000);
 *   // Use data...
 *   // No need to free - reset() handles it
 */
class LIMBO_API FrameAllocator {
public:
    /**
     * Create a frame allocator with specified capacity
     * @param capacityBytes Size of the memory pool in bytes
     */
    explicit FrameAllocator(usize capacityBytes = 1024 * 1024);

    ~FrameAllocator();

    // Non-copyable
    FrameAllocator(const FrameAllocator&) = delete;
    FrameAllocator& operator=(const FrameAllocator&) = delete;

    // Movable
    FrameAllocator(FrameAllocator&& other) noexcept;
    FrameAllocator& operator=(FrameAllocator&& other) noexcept;

    /**
     * Allocate memory with specified alignment
     * @param size Bytes to allocate
     * @param alignment Alignment requirement (must be power of 2, default 16)
     * @return Pointer to allocated memory, or nullptr if full
     */
    [[nodiscard]] void* allocate(usize size, usize alignment = 16);

    /**
     * Allocate and default-construct an object
     * @tparam T Type to construct
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to constructed object, or nullptr if full
     */
    template <typename T, typename... Args>
    [[nodiscard]] T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        if (!mem) {
            return nullptr;
        }
        return new (mem) T(std::forward<Args>(args)...);
    }

    /**
     * Allocate an array (no constructors called - use for POD types)
     * @tparam T Element type
     * @param count Number of elements
     * @return Pointer to array, or nullptr if full
     */
    template <typename T>
    [[nodiscard]] T* allocateArray(usize count) {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }

    /**
     * Reset the allocator for a new frame
     * All previously allocated memory becomes invalid after this call
     * This is O(1) - just resets the offset pointer
     */
    void reset();

    /**
     * Get current memory usage in bytes
     */
    [[nodiscard]] usize getUsedBytes() const { return m_offset; }

    /**
     * Get total capacity in bytes
     */
    [[nodiscard]] usize getCapacity() const { return m_capacity; }

    /**
     * Get remaining capacity in bytes
     */
    [[nodiscard]] usize getRemainingBytes() const { return m_capacity - m_offset; }

    /**
     * Get usage as a percentage (0.0 - 1.0)
     */
    [[nodiscard]] f32 getUsagePercent() const {
        return m_capacity > 0 ? static_cast<f32>(m_offset) / static_cast<f32>(m_capacity) : 0.0f;
    }

    /**
     * Get peak usage (high-water mark) in bytes
     */
    [[nodiscard]] usize getPeakUsage() const { return m_peakUsage; }

    /**
     * Check if an allocation of given size would succeed
     * @param size Bytes to allocate
     * @param alignment Alignment requirement
     * @return True if allocation would succeed
     */
    [[nodiscard]] bool canAllocate(usize size, usize alignment = 16) const;

private:
    u8* m_buffer = nullptr;
    usize m_capacity = 0;
    usize m_offset = 0;
    usize m_peakUsage = 0;
};

// ============================================================================
// Global Frame Allocator
// ============================================================================

/**
 * Global frame allocator access
 *
 * The global frame allocator is initialized by Application and reset
 * at the start of each frame automatically.
 */
namespace frame {

/**
 * Initialize the global frame allocator
 * @param capacityBytes Size of the memory pool (default 1MB)
 */
LIMBO_API void init(usize capacityBytes = 1024 * 1024);

/**
 * Shutdown the global frame allocator
 */
LIMBO_API void shutdown();

/**
 * Reset the global frame allocator (called at frame start)
 */
LIMBO_API void reset();

/**
 * Get the global frame allocator
 */
LIMBO_API FrameAllocator& get();

/**
 * Check if the global frame allocator is initialized
 */
LIMBO_API bool isInitialized();

/**
 * Allocate and construct an object using the global frame allocator
 */
template <typename T, typename... Args>
[[nodiscard]] T* create(Args&&... args) {
    return get().create<T>(std::forward<Args>(args)...);
}

/**
 * Allocate an array using the global frame allocator
 */
template <typename T>
[[nodiscard]] T* allocateArray(usize count) {
    return get().allocateArray<T>(count);
}

}  // namespace frame

// ============================================================================
// FrameVector - std::vector-like container using frame allocator
// ============================================================================

/**
 * FrameVector - A vector-like container that uses the frame allocator
 *
 * WARNING: Only valid for the current frame! Do not store references
 * to FrameVector data across frames.
 *
 * This is a drop-in replacement for std::vector in hot paths where
 * you need temporary storage that doesn't survive the frame.
 */
template <typename T>
class FrameVector {
public:
    using value_type = T;
    using size_type = usize;
    using iterator = T*;
    using const_iterator = const T*;

    FrameVector() = default;

    /**
     * Reserve capacity for at least 'capacity' elements
     */
    void reserve(usize capacity) {
        if (capacity <= m_capacity) {
            return;
        }

        T* newData = frame::allocateArray<T>(capacity);
        if (!newData) {
            return;  // Allocation failed
        }

        // Copy existing elements
        if (m_data && m_size > 0) {
            for (usize i = 0; i < m_size; ++i) {
                new (&newData[i]) T(std::move(m_data[i]));
            }
        }

        m_data = newData;
        m_capacity = capacity;
    }

    /**
     * Add an element to the end
     */
    void push_back(const T& value) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 16 : m_capacity * 2);
        }
        if (m_size < m_capacity) {
            new (&m_data[m_size]) T(value);
            m_size++;
        }
    }

    /**
     * Add an element to the end (move)
     */
    void push_back(T&& value) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 16 : m_capacity * 2);
        }
        if (m_size < m_capacity) {
            new (&m_data[m_size]) T(std::move(value));
            m_size++;
        }
    }

    /**
     * Construct an element in place at the end
     */
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (m_size >= m_capacity) {
            reserve(m_capacity == 0 ? 16 : m_capacity * 2);
        }
        new (&m_data[m_size]) T(std::forward<Args>(args)...);
        return m_data[m_size++];
    }

    /**
     * Clear the vector (doesn't free memory - that happens at frame reset)
     */
    void clear() { m_size = 0; }

    // Element access
    T& operator[](usize index) { return m_data[index]; }
    const T& operator[](usize index) const { return m_data[index]; }

    T& front() { return m_data[0]; }
    const T& front() const { return m_data[0]; }

    T& back() { return m_data[m_size - 1]; }
    const T& back() const { return m_data[m_size - 1]; }

    T* data() { return m_data; }
    const T* data() const { return m_data; }

    // Iterators
    iterator begin() { return m_data; }
    iterator end() { return m_data + m_size; }
    const_iterator begin() const { return m_data; }
    const_iterator end() const { return m_data + m_size; }
    const_iterator cbegin() const { return m_data; }
    const_iterator cend() const { return m_data + m_size; }

    // Capacity
    [[nodiscard]] usize size() const { return m_size; }
    [[nodiscard]] usize capacity() const { return m_capacity; }
    [[nodiscard]] bool empty() const { return m_size == 0; }

private:
    T* m_data = nullptr;
    usize m_size = 0;
    usize m_capacity = 0;
};

}  // namespace limbo
