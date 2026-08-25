// #include "engine/key_encoder.hpp"
// #include <iostream>
// #include <string>
// #include <unordered_map>
// #include <vector>
// #include <fstream>
// #include <iomanip>
// #include <map>

// using namespace TSDB;

// // Symbol Table: Maps string names to uint32_t IDs
// std::unordered_map<std::string, uint32_t> string_dictionary;
// uint32_t next_id = 1;

// uint32_t get_id_for_string(const std::string& text) {
//     if (string_dictionary.find(text) == string_dictionary.end()) {
//         string_dictionary[text] = next_id++;
//     }
//     return string_dictionary[text];
// }

// int main() {
//     // MemTable: Stores raw encoded byte keys and values in sorted order
//     std::map<std::vector<uint8_t>, std::vector<uint8_t>, KeyComparator> memtable;

//     std::cout << "===========================================\n";
//     std::cout << "  TSDB Interactive CLI Pipeline Tester     \n";
//     std::cout << "===========================================\n\n";

//     while (true) {
//         std::string metric, host;
//         uint64_t timestamp;
//         double value;

//         std::cout << "Enter Metric Name (or 'exit' to quit): ";
//         std::cin >> metric;
//         if (metric == "exit") break;

//         std::cout << "Enter Host Name (e.g., server-01): ";
//         std::cin >> host;

//         std::cout << "Enter Timestamp (e.g., 1700000000): ";
//         std::cin >> timestamp;

//         std::cout << "Enter Value (e.g., 37.5): ";
//         std::cin >> value;

//         // STEP 1: Map Strings to IDs
//         uint32_t metric_id = get_id_for_string(metric);
//         uint32_t host_id   = get_id_for_string(host);

//         // STEP 2: Component 1 Encoding
//         SliceKey key{metric_id, host_id, timestamp};
//         SliceValue val{value};

//         std::vector<uint8_t> encoded_key = key.encode(); // 16 Bytes Big-Endian
//         std::vector<uint8_t> encoded_val = val.encode(); // 8 Bytes Big-Endian

//         // STEP 3: Write Raw Bytes to WAL File
//         std::ofstream wal("tsdb.wal", std::ios::binary | std::ios::app);
//         wal.write(reinterpret_cast<char*>(encoded_key.data()), encoded_key.size());
//         wal.write(reinterpret_cast<char*>(encoded_val.data()), encoded_val.size());
//         wal.close();

//         // STEP 4: Insert Same Raw Bytes into MemTable
//         memtable[encoded_key] = encoded_val;

//         // Print Feedback
//         std::cout << "\n-------------------------------------------\n";
//         std::cout << "[ENCODED KEY (16B)]: ";
//         for (int b : encoded_key) std::cout << std::hex << std::setw(2) << std::setfill('0') << b << " ";
        
//         std::cout << "\n[ENCODED VAL (8B) ]: ";
//         for (int b : encoded_val) std::cout << std::hex << std::setw(2) << std::setfill('0') << b << " ";
//         std::cout << std::dec << "\n";

//         std::cout << "[WAL STATUS]      : 24 raw bytes appended to 'tsdb.wal'\n";
//         std::cout << "[MEMTABLE STATUS] : Key/Value inserted. Total items in MemTable: " << memtable.size() << "\n";
//         std::cout << "-------------------------------------------\n\n";
//     }

//     std::cout << "Exiting. Component 1 pipeline fully verified.\n";
//     return 0;
// }


#include "engine/db.hpp"
#include <iostream>

int main() {
    using namespace TSDB;

    // Initialize database instance
    Database db("tsdb.wal");

    std::cout << "--- Executing db.put() Calls ---\n";

    // Clean API calls
    db.put("temperature", "server-01", 1700000000000ULL, 37.5);
    db.put("cpu_usage",   "server-01", 1700000001000ULL, 88.2);
    db.put("stock_price", "nasdaq",    1700000002000ULL, 180.50);

    std::cout << "\nTotal records active in MemTable: " << db.memtable_size() << "\n";

    return 0;
}