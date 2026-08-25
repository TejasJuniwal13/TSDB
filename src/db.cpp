#include "engine/db.hpp"
#include <iostream>

namespace TSDB {

Database::Database(const std::string& wal_path) : wal_(wal_path) {
    // 1. RECOVERY: Replay WAL binary entries on startup into MemTable
    auto recovered_records = wal_.recover();
    for (const auto& [key, val] : recovered_records) {
        memtable_[key] = val;
    }
    if (!recovered_records.empty()) {
        std::cout << "[Database] Successfully restored " << recovered_records.size() 
                  << " records from WAL on startup.\n";
    }
}

void Database::put(const std::string& metric_name, 
                   const std::string& host_name, 
                   uint64_t timestamp, 
                   double value) {
    
    // Step 1: Map human strings to 32-bit IDs
    uint32_t metric_id = catalog_.get_or_create(metric_name);
    uint32_t host_id   = catalog_.get_or_create(host_name);

    // Step 2: Component 1 Encodes struct to Big-Endian std::vector<uint8_t>
    SliceKey key_struct{metric_id, host_id, timestamp};
    SliceValue val_struct{value};

    std::vector<uint8_t> encoded_key = key_struct.encode(); // Returns 16 bytes
    std::vector<uint8_t> encoded_val = val_struct.encode(); // Returns 8 bytes

    // Step 3: Component 2 WAL framing, disk append, and fdatasync
    wal_.append(encoded_key, encoded_val);

    // Step 4: Insert raw byte vectors into MemTable ONLY after disk confirms durability
    memtable_[encoded_key] = encoded_val;
}

} // namespace TSDB