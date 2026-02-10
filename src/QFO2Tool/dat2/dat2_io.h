#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace dat2 {

// Read a little-endian u32 from buf at offset.
// Returns false if offset + 4 > buf_size (out of bounds).
bool read_le_u32(const uint8_t* buf, size_t buf_size, size_t offset, uint32_t& out);

// Read a single byte from buf at offset.
// Returns false if offset >= buf_size (out of bounds).
bool read_le_u8(const uint8_t* buf, size_t buf_size, size_t offset, uint8_t& out);

// Append a little-endian u32 to a byte vector.
void write_le_u32(std::vector<uint8_t>& buf, uint32_t val);

// Append a single byte to a byte vector.
void write_le_u8(std::vector<uint8_t>& buf, uint8_t val);

} // namespace dat2
