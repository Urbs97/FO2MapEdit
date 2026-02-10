#include "dat2_io.h"

namespace dat2 {

bool read_le_u32(const uint8_t* buf, size_t buf_size, size_t offset, uint32_t& out)
{
    if (offset + 4 > buf_size) { return false;
}
    out = static_cast<uint32_t>(buf[offset])
        | (static_cast<uint32_t>(buf[offset + 1]) << 8)
        | (static_cast<uint32_t>(buf[offset + 2]) << 16)
        | (static_cast<uint32_t>(buf[offset + 3]) << 24);
    return true;
}

bool read_le_u8(const uint8_t* buf, size_t buf_size, size_t offset, uint8_t& out)
{
    if (offset >= buf_size) { return false;
}
    out = buf[offset];
    return true;
}

} // namespace dat2
