#ifndef ENGINE_KEY_ENCODER_HPP
#define ENGINE_KEY_ENCODER_HPP

#include <cstdint>
#include <vector>
#include <cstddef>

namespace TSDB {

// Structured metadata key
struct SliceKey {
    uint32_t metric_id;
    uint32_t host_id;
    uint64_t timestamp;

    static constexpr size_t KEY_SIZE = 16;

    // Serializes internal C++ types into a 16-byte Big-Endian buffer
    std::vector<uint8_t> encode() const;

    // Deserializes a 16-byte Big-Endian buffer back into C++ struct
    static SliceKey decode(const uint8_t* buffer);
};

// Double-precision floating point telemetry value (IEEE 754)
struct SliceValue {
    double value;

    static constexpr size_t VALUE_SIZE = 8;

    std::vector<uint8_t> encode() const;
    static SliceValue decode(const uint8_t* buffer);
};

// Zero-dependency memory comparator for byte keys
struct KeyComparator {
    bool operator()(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) const;
};

} // namespace TSDB

#endif // ENGINE_KEY_ENCODER_HPP