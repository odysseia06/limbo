#include "limbo/core/UUID.hpp"

#include <iomanip>
#include <random>
#include <sstream>

namespace limbo {

namespace {

// Thread-local random generator for UUID v4
std::mt19937_64& getRandomGenerator() {
    static thread_local std::mt19937_64 generator{std::random_device{}()};
    return generator;
}

u8 hexCharToNibble(char c) {
    if (c >= '0' && c <= '9')
        return static_cast<u8>(c - '0');
    if (c >= 'a' && c <= 'f')
        return static_cast<u8>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F')
        return static_cast<u8>(c - 'A' + 10);
    return 0;
}

char nibbleToHexChar(u8 nibble) {
    return "0123456789abcdef"[nibble & 0x0F];
}

}  // namespace

UUID UUID::generate() {
    auto& gen = getRandomGenerator();
    std::uniform_int_distribution<u64> dist;

    u64 high = dist(gen);
    u64 low = dist(gen);

    // Set version to 4 (random UUID)
    // Version is in bits 12-15 of the high part (byte 6, high nibble)
    high = (high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;

    // Set variant to RFC 4122 (bits 6-7 of byte 8 should be 10)
    low = (low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    return UUID{high, low};
}

UUID UUID::fromString(StringView str) {
    // Remove dashes if present
    String compact;
    compact.reserve(32);

    for (char c : str) {
        if (c != '-') {
            compact += c;
        }
    }

    if (compact.size() != 32) {
        return UUID::null();
    }

    // Parse hex string into bytes
    u64 high = 0;
    u64 low = 0;

    for (size_t i = 0; i < 16; ++i) {
        u8 byte = static_cast<u8>((hexCharToNibble(compact[i * 2]) << 4) |
                                   hexCharToNibble(compact[i * 2 + 1]));
        if (i < 8) {
            high = (high << 8) | byte;
        } else {
            low = (low << 8) | byte;
        }
    }

    return UUID{high, low};
}

String UUID::toString() const {
    // Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    String result;
    result.reserve(36);

    auto appendByte = [&result](u8 byte) {
        result += nibbleToHexChar(static_cast<u8>(byte >> 4));
        result += nibbleToHexChar(byte & 0x0F);
    };

    // High part: 8-4-4 (bytes 7-4, 3-2, 1-0)
    for (int i = 7; i >= 4; --i) {
        appendByte(static_cast<u8>(m_high >> (i * 8)));
    }
    result += '-';

    for (int i = 3; i >= 2; --i) {
        appendByte(static_cast<u8>(m_high >> (i * 8)));
    }
    result += '-';

    for (int i = 1; i >= 0; --i) {
        appendByte(static_cast<u8>(m_high >> (i * 8)));
    }
    result += '-';

    // Low part: 4-12 (bytes 7-6, 5-0)
    for (int i = 7; i >= 6; --i) {
        appendByte(static_cast<u8>(m_low >> (i * 8)));
    }
    result += '-';

    for (int i = 5; i >= 0; --i) {
        appendByte(static_cast<u8>(m_low >> (i * 8)));
    }

    return result;
}

String UUID::toCompactString() const {
    String result;
    result.reserve(32);

    auto appendByte = [&result](u8 byte) {
        result += nibbleToHexChar(byte >> 4);
        result += nibbleToHexChar(byte & 0x0F);
    };

    for (int i = 7; i >= 0; --i) {
        appendByte(static_cast<u8>(m_high >> (i * 8)));
    }
    for (int i = 7; i >= 0; --i) {
        appendByte(static_cast<u8>(m_low >> (i * 8)));
    }

    return result;
}

}  // namespace limbo
