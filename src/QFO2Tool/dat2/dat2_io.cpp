#include "dat2_io.h"

namespace dat2 {

bool read_le_u32(const uint8_t* buf, size_t buf_size, size_t offset, uint32_t& out) {
    if (offset + 4 > buf_size) {
        return false;
    }
    out = static_cast<uint32_t>(buf[offset]) | (static_cast<uint32_t>(buf[offset + 1]) << 8) |
          (static_cast<uint32_t>(buf[offset + 2]) << 16) |
          (static_cast<uint32_t>(buf[offset + 3]) << 24);
    return true;
}

bool read_le_u8(const uint8_t* buf, size_t buf_size, size_t offset, uint8_t& out) {
    if (offset >= buf_size) {
        return false;
    }
    out = buf[offset];
    return true;
}

void write_le_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val));
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val >> 16));
    buf.push_back(static_cast<uint8_t>(val >> 24));
}

void write_le_u8(std::vector<uint8_t>& buf, uint8_t val) { buf.push_back(val); }

} // namespace dat2
