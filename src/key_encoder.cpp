#include "engine/key_encoder.hpp"
#include <cstring>
#include <stdexcept>

namespace TSDB {

std::vector<uint8_t> SliceKey::encode() const {
    std::vector<uint8_t> buf(KEY_SIZE);

    // Metric ID (Bytes 0-3): Shift most significant bits to the left
    buf[0] = static_cast<uint8_t>((metric_id >> 24) & 0xFF);
    buf[1] = static_cast<uint8_t>((metric_id >> 16) & 0xFF);
    buf[2] = static_cast<uint8_t>((metric_id >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>(metric_id & 0xFF);

    // Host ID (Bytes 4-7)
    buf[4] = static_cast<uint8_t>((host_id >> 24) & 0xFF);
    buf[5] = static_cast<uint8_t>((host_id >> 16) & 0xFF);
    buf[6] = static_cast<uint8_t>((host_id >> 8) & 0xFF);
    buf[7] = static_cast<uint8_t>(host_id & 0xFF);

    // Timestamp (Bytes 8-15)
    buf[8]  = static_cast<uint8_t>((timestamp >> 56) & 0xFF);
    buf[9]  = static_cast<uint8_t>((timestamp >> 48) & 0xFF);
    buf[10] = static_cast<uint8_t>((timestamp >> 40) & 0xFF);
    buf[11] = static_cast<uint8_t>((timestamp >> 32) & 0xFF);
    buf[12] = static_cast<uint8_t>((timestamp >> 24) & 0xFF);
    buf[13] = static_cast<uint8_t>((timestamp >> 16) & 0xFF);
    buf[14] = static_cast<uint8_t>((timestamp >> 8) & 0xFF);
    buf[15] = static_cast<uint8_t>(timestamp & 0xFF);

    return buf;
}

SliceKey SliceKey::decode(const uint8_t* buffer) {
    if (!buffer) {
        throw std::invalid_argument("Null buffer passed to SliceKey::decode");
    }

    SliceKey key;

    // Reconstruct integers by bitwise ORing shifted bytes
    key.metric_id = (static_cast<uint32_t>(buffer[0]) << 24) |
                    (static_cast<uint32_t>(buffer[1]) << 16) |
                    (static_cast<uint32_t>(buffer[2]) << 8)  |
                    (static_cast<uint32_t>(buffer[3]));

    key.host_id   = (static_cast<uint32_t>(buffer[4]) << 24) |
                    (static_cast<uint32_t>(buffer[5]) << 16) |
                    (static_cast<uint32_t>(buffer[6]) << 8)  |
                    (static_cast<uint32_t>(buffer[7]));

    key.timestamp = (static_cast<uint64_t>(buffer[8])  << 56) |
                    (static_cast<uint64_t>(buffer[9])  << 48) |
                    (static_cast<uint64_t>(buffer[10]) << 40) |
                    (static_cast<uint64_t>(buffer[11]) << 32) |
                    (static_cast<uint64_t>(buffer[12]) << 24) |
                    (static_cast<uint64_t>(buffer[13]) << 16) |
                    (static_cast<uint64_t>(buffer[14]) << 8)  |
                    (static_cast<uint64_t>(buffer[15]));

    return key;
}

std::vector<uint8_t> SliceValue::encode() const {
    std::vector<uint8_t> buf(VALUE_SIZE);
    
    // Copy bit representation of double into 64-bit unsigned int
    uint64_t raw_bits;
    std::memcpy(&raw_bits, &value, sizeof(double));

    buf[0] = static_cast<uint8_t>((raw_bits >> 56) & 0xFF);
    buf[1] = static_cast<uint8_t>((raw_bits >> 48) & 0xFF);
    buf[2] = static_cast<uint8_t>((raw_bits >> 40) & 0xFF);
    buf[3] = static_cast<uint8_t>((raw_bits >> 32) & 0xFF);
    buf[4] = static_cast<uint8_t>((raw_bits >> 24) & 0xFF);
    buf[5] = static_cast<uint8_t>((raw_bits >> 16) & 0xFF);
    buf[6] = static_cast<uint8_t>((raw_bits >> 8) & 0xFF);
    buf[7] = static_cast<uint8_t>(raw_bits & 0xFF);

    return buf;
}

SliceValue SliceValue::decode(const uint8_t* buffer) {
    if (!buffer) {
        throw std::invalid_argument("Null buffer passed to SliceValue::decode");
    }

    uint64_t raw_bits = (static_cast<uint64_t>(buffer[0]) << 56) |
                        (static_cast<uint64_t>(buffer[1]) << 48) |
                        (static_cast<uint64_t>(buffer[2]) << 40) |
                        (static_cast<uint64_t>(buffer[3]) << 32) |
                        (static_cast<uint64_t>(buffer[4]) << 24) |
                        (static_cast<uint64_t>(buffer[5]) << 16) |
                        (static_cast<uint64_t>(buffer[6]) << 8)  |
                        (static_cast<uint64_t>(buffer[7]));

    SliceValue val;
    std::memcpy(&val.value, &raw_bits, sizeof(double));
    return val;
}

bool KeyComparator::operator()(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) const {
    return std::memcmp(a.data(), b.data(), SliceKey::KEY_SIZE) < 0;
}

} // namespace TSDB 