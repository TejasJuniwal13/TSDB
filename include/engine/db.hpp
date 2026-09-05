#ifndef ENGINE_DB_HPP
#define ENGINE_DB_HPP

#include "memtable.hpp"
#include "engine/key_encoder.hpp"
#include "engine/wal.hpp" // 1. Include the WAL header
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace TSDB {

class StringCatalog {
private:
    std::unordered_map<std::string, uint32_t> string_to_id_;
    uint32_t next_id_ = 1;

public:
    uint32_t get_or_create(const std::string& name) {
        auto it = string_to_id_.find(name);
        if (it != string_to_id_.end()) {
            return it->second;
        }
        uint32_t id = next_id_++;
        string_to_id_[name] = id;
        return id;
    }
};

class Database {
private:
    StringCatalog catalog_;
    WriteAheadLog wal_; // 2. Declare wal_ member variable here
    // std::map<std::vector<uint8_t>, std::vector<uint8_t>, KeyComparator> memtable_;
    MemTable memtable_;
    static constexpr size_t MEMTABLE_THRESHOLD = 2*1024*1024; // 2MB 

public:
    explicit Database(const std::string& wal_path = "tsdb.wal");
    ~Database() = default;

    void put(const std::string& metric_name, 
             const std::string& host_name, 
             uint64_t timestamp, 
             double value);

    // size_t memtable_size() const { return memtable_. }
    bool get(const std::string& metric_name,
             const std::string& host_name,
             uint64_t timestamp,
             double &value_out);

    size_t size_bytes() const{
        return memtable_.size_bytes();
    }
    
};

} // namespace TSDB

#endif // ENGINE_DB_HPP