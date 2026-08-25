#ifndef ENGINE_WAL_HPP
#define ENGINE_WAL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace TSDB {

class WriteAheadLog {
public:
    static constexpr uint8_t RECORD_TYPE_PUT = 0x01;
    static constexpr uint8_t RECORD_TYPE_DELETE = 0x02;
    static constexpr size_t HEADER_SIZE = 7; // 4B CRC32 + 2B Length + 1B Type

private:
    int fd_ = -1;
    std::string path_;

    // Zero-dependency IEEE 802.3 CRC32 calculator
    static uint32_t compute_crc32(const uint8_t* data, size_t length);

public:
    explicit WriteAheadLog(const std::string& path);
    ~WriteAheadLog();

    // Prevent copying to avoid multiple file descriptor collisions
    WriteAheadLog(const WriteAheadLog&) = delete;
    WriteAheadLog& operator=(const WriteAheadLog&) = delete;

    // Packs binary frame, writes to OS page cache, and forces physical sync
    void append(const std::vector<uint8_t>& key, const std::vector<uint8_t>& value);

    // Replays log frames on boot and truncates corrupted tail writes
    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> recover();

    void sync();
    void close();
};

} // namespace TSDB

#endif // ENGINE_WAL_HPP    