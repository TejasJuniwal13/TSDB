#ifndef ENGINE_MEMTABLE_HPP
#define ENGINE_MEMTABLE_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <random>
#include <utility>

namespace TSDB {

/**
 * Node structure for the SkipList.
 * Stores binary key-value byte arrays and forward level pointers.
 */
struct SkipNode {
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
    std::vector<SkipNode*> forward;

    SkipNode(std::vector<uint8_t> k, std::vector<uint8_t> v, int level)
        : key(std::move(k)), value(std::move(v)), forward(level, nullptr) {}
};

/**
 * SkipList-backed MemTable providing O(log N) point lookups,
 * O(log N) insertions, and O(1) sequential level 0 range scans.
 */
class MemTable {
private:
    static constexpr int MAX_LEVEL = 16;
    static constexpr float P = 0.5f; // Geometric coin-flip probability

    SkipNode* head_;
    int current_level_;
    size_t size_bytes_;
    size_t element_count_;

    // Thread-safe / fast pseudorandom generator for level generation
    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<float> dist_;

    int random_level() const;

public:
    MemTable();
    ~MemTable();

    // Prevent copying to enforce RAII pointer ownership
    MemTable(const MemTable&) = delete;
    MemTable& operator=(const MemTable&) = delete;

    // Core API
    void put(const std::vector<uint8_t>& key, const std::vector<uint8_t>& value);
    bool get(const std::vector<uint8_t>& key, std::vector<uint8_t>& value_out) const;
    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> scan(
        const std::vector<uint8_t>& start_key, 
        const std::vector<uint8_t>& end_key) const;

    void clear();

    // Accessors
    size_t size_bytes() const { return size_bytes_; }
    size_t element_count() const { return element_count_; }
    SkipNode* get_head() const { return head_; }
};

} // namespace TSDB

#endif // ENGINE_MEMTABLE_HPP