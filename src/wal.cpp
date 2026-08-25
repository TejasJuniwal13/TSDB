#include "engine/wal.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cstring>
#include <stdexcept>
#include <iostream>

namespace TSDB {

uint32_t WriteAheadLog::compute_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

WriteAheadLog::WriteAheadLog(const std::string& path) : path_(path) {
    fd_ = ::open(path_.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to open WAL file: " + path_);
    }
}

WriteAheadLog::~WriteAheadLog() {
    close();
}

void WriteAheadLog::close() {
    if (fd_ >= 0) {
        ::fdatasync(fd_);
        ::close(fd_);
        fd_ = -1;
    }
}

void WriteAheadLog::sync() {
    if (fd_ >= 0) {
        if (::fdatasync(fd_) < 0) {
            throw std::runtime_error("fdatasync failed on WAL file");
        }
    }
}

void WriteAheadLog::append(const std::vector<uint8_t>& key, const std::vector<uint8_t>& value) {
    uint16_t payload_len = static_cast<uint16_t>(key.size() + value.size());
    size_t frame_size = HEADER_SIZE + payload_len;

    std::vector<uint8_t> frame(frame_size);

    // 1. Pack Length (2 Bytes - Big Endian) into Header
    frame[4] = static_cast<uint8_t>((payload_len >> 8) & 0xFF);
    frame[5] = static_cast<uint8_t>(payload_len & 0xFF);

    // 2. Pack Record Type (1 Byte)
    frame[6] = RECORD_TYPE_PUT;

    // 3. Pack Payload (16-Byte Key + 8-Byte Value)
    std::memcpy(frame.data() + HEADER_SIZE, key.data(), key.size());
    std::memcpy(frame.data() + HEADER_SIZE + key.size(), value.data(), value.size());

    // 4. Compute CRC32 over [Length (2B) + Type (1B) + Payload (24B)]
    uint32_t crc = compute_crc32(frame.data() + 4, 3 + payload_len);

    // 5. Pack CRC32 (4 Bytes - Big Endian) at byte offsets 0-3
    frame[0] = static_cast<uint8_t>((crc >> 24) & 0xFF);
    frame[1] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    frame[2] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    frame[3] = static_cast<uint8_t>(crc & 0xFF);

    // 6. POSIX append write to kernel page cache
    ssize_t bytes_written = ::write(fd_, frame.data(), frame.size());
    if (bytes_written != static_cast<ssize_t>(frame.size())) {
        throw std::runtime_error("Torn write encountered during WAL append");
    }

    // 7. Physical drive flush (Ensures zero data loss)
    sync();
}

std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> WriteAheadLog::recover() {
    std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> recovered_records;

    ::lseek(fd_, 0, SEEK_SET);

    while (true) {
        off_t record_start_offset = ::lseek(fd_, 0, SEEK_CUR);

        uint8_t header[HEADER_SIZE];
        ssize_t header_bytes = ::read(fd_, header, HEADER_SIZE);

        if (header_bytes == 0) {
            break; // Reached clean End-Of-File
        }

        if (header_bytes < static_cast<ssize_t>(HEADER_SIZE)) {
            // Check return code of ftruncate to pass -Werror
            if (::ftruncate(fd_, record_start_offset) != 0) {
                throw std::runtime_error("Failed to truncate WAL file at torn header");
            }
            break;
        }

        uint32_t expected_crc = (static_cast<uint32_t>(header[0]) << 24) |
                                (static_cast<uint32_t>(header[1]) << 16) |
                                (static_cast<uint32_t>(header[2]) << 8)  |
                                (static_cast<uint32_t>(header[3]));

        uint16_t payload_len = (static_cast<uint16_t>(header[4]) << 8) |
                               (static_cast<uint16_t>(header[5]));

        std::vector<uint8_t> payload(payload_len);
        ssize_t payload_bytes = ::read(fd_, payload.data(), payload_len);

        if (payload_bytes < static_cast<ssize_t>(payload_len)) {
            if (::ftruncate(fd_, record_start_offset) != 0) {
                throw std::runtime_error("Failed to truncate WAL file at torn payload");
            }
            break;
        }

        // Validate framing checksum
        std::vector<uint8_t> check_buf(3 + payload_len);
        check_buf[0] = header[4];
        check_buf[1] = header[5];
        check_buf[2] = header[6];
        std::memcpy(check_buf.data() + 3, payload.data(), payload_len);

        uint32_t actual_crc = compute_crc32(check_buf.data(), check_buf.size());

        if (actual_crc != expected_crc) {
            if (::ftruncate(fd_, record_start_offset) != 0) {
                throw std::runtime_error("Failed to truncate WAL file on CRC mismatch");
            }
            break;
        }

        if (payload_len == 24) {
            std::vector<uint8_t> key(payload.begin(), payload.begin() + 16);
            std::vector<uint8_t> val(payload.begin() + 16, payload.end());
            recovered_records.emplace_back(key, val);
        }
    }

    ::lseek(fd_, 0, SEEK_END);
    return recovered_records;
}

} // namespace TSDB