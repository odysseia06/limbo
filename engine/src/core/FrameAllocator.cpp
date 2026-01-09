#include "limbo/core/FrameAllocator.hpp"

#include "limbo/core/Assert.hpp"
#include "limbo/debug/Log.hpp"

#include <cstdlib>
#include <utility>

namespace limbo {

// ============================================================================
// FrameAllocator Implementation
// ============================================================================

FrameAllocator::FrameAllocator(usize capacityBytes) : m_capacity(capacityBytes) {
    if (capacityBytes > 0) {
        // Allocate aligned memory (16-byte aligned for SIMD-friendly access)
#if defined(_MSC_VER) || defined(__MINGW32__)
        m_buffer = static_cast<u8*>(_aligned_malloc(capacityBytes, 16));
#else
        m_buffer = static_cast<u8*>(std::aligned_alloc(16, capacityBytes));
#endif
        LIMBO_ASSERT(m_buffer != nullptr, "Failed to allocate frame allocator buffer");
    }
}

FrameAllocator::~FrameAllocator() {
    if (m_buffer) {
#if defined(_MSC_VER) || defined(__MINGW32__)
        _aligned_free(m_buffer);
#else
        std::free(m_buffer);
#endif
        m_buffer = nullptr;
    }
}

FrameAllocator::FrameAllocator(FrameAllocator&& other) noexcept
    : m_buffer(other.m_buffer),
      m_capacity(other.m_capacity),
      m_offset(other.m_offset),
      m_peakUsage(other.m_peakUsage) {
    other.m_buffer = nullptr;
    other.m_capacity = 0;
    other.m_offset = 0;
    other.m_peakUsage = 0;
}

FrameAllocator& FrameAllocator::operator=(FrameAllocator&& other) noexcept {
    if (this != &other) {
        // Free existing buffer
        if (m_buffer) {
#if defined(_MSC_VER) || defined(__MINGW32__)
            _aligned_free(m_buffer);
#else
            std::free(m_buffer);
#endif
        }

        // Move from other
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_offset = other.m_offset;
        m_peakUsage = other.m_peakUsage;

        // Clear other
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_offset = 0;
        other.m_peakUsage = 0;
    }
    return *this;
}

void* FrameAllocator::allocate(usize size, usize alignment) {
    if (size == 0 || !m_buffer) {
        return nullptr;
    }

    // Alignment must be power of 2
    LIMBO_ASSERT((alignment & (alignment - 1)) == 0, "Alignment must be power of 2");

    // Calculate aligned offset
    usize currentAddr = reinterpret_cast<usize>(m_buffer + m_offset);
    usize alignedAddr = (currentAddr + alignment - 1) & ~(alignment - 1);
    usize alignmentPadding = alignedAddr - currentAddr;

    // Check if we have enough space
    usize totalSize = alignmentPadding + size;
    if (m_offset + totalSize > m_capacity) {
        LIMBO_LOG_CORE_WARN("FrameAllocator: Out of memory! Requested {} bytes, have {} remaining",
                           size, getRemainingBytes());
        return nullptr;
    }

    // Bump the offset
    m_offset += totalSize;

    // Update peak usage
    if (m_offset > m_peakUsage) {
        m_peakUsage = m_offset;
    }

    return reinterpret_cast<void*>(alignedAddr);
}

void FrameAllocator::reset() {
    m_offset = 0;
    // Note: We don't reset peak usage - it's tracked across the lifetime
}

bool FrameAllocator::canAllocate(usize size, usize alignment) const {
    if (size == 0 || !m_buffer) {
        return false;
    }

    usize currentAddr = reinterpret_cast<usize>(m_buffer + m_offset);
    usize alignedAddr = (currentAddr + alignment - 1) & ~(alignment - 1);
    usize alignmentPadding = alignedAddr - currentAddr;

    return (m_offset + alignmentPadding + size) <= m_capacity;
}

// ============================================================================
// Global Frame Allocator
// ============================================================================

namespace {

FrameAllocator* g_globalFrameAllocator = nullptr;

}  // namespace

namespace frame {

void init(usize capacityBytes) {
    if (g_globalFrameAllocator) {
        LIMBO_LOG_CORE_WARN("FrameAllocator: Already initialized, ignoring init call");
        return;
    }

    g_globalFrameAllocator = new FrameAllocator(capacityBytes);
    LIMBO_LOG_CORE_DEBUG("FrameAllocator: Initialized with {} KB capacity", capacityBytes / 1024);
}

void shutdown() {
    if (g_globalFrameAllocator) {
        LIMBO_LOG_CORE_DEBUG("FrameAllocator: Peak usage was {} KB",
                            g_globalFrameAllocator->getPeakUsage() / 1024);
        delete g_globalFrameAllocator;
        g_globalFrameAllocator = nullptr;
    }
}

void reset() {
    if (g_globalFrameAllocator) {
        g_globalFrameAllocator->reset();
    }
}

FrameAllocator& get() {
    LIMBO_ASSERT(g_globalFrameAllocator != nullptr, "FrameAllocator not initialized");
    return *g_globalFrameAllocator;
}

bool isInitialized() {
    return g_globalFrameAllocator != nullptr;
}

}  // namespace frame

}  // namespace limbo
