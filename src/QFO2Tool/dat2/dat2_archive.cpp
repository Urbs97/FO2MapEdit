#include "dat2_archive.h"
#include "dat2_io.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <zlib.h>

namespace dat2 {

const char* dat2_error_str(Dat2Error err)
{
    switch (err) {
        case Dat2Error::OK:                  return "OK";
        case Dat2Error::FILE_OPEN_FAILED:    return "failed to open file";
        case Dat2Error::FILE_READ_FAILED:    return "failed to read file";
        case Dat2Error::FILE_TOO_SMALL:      return "file too small to be a valid DAT2 archive";
        case Dat2Error::FILE_SIZE_MISMATCH:  return "file size does not match DAT2 footer";
        case Dat2Error::TREE_SIZE_INVALID:   return "tree size is invalid";
        case Dat2Error::TREE_PARSE_ERROR:    return "failed to parse tree entry";
        case Dat2Error::ENTRY_OUT_OF_BOUNDS: return "entry data is out of bounds";
        case Dat2Error::DECOMPRESSION_FAILED:return "zlib decompression failed";
    }
    return "unknown error";
}

// ── helpers ──────────────────────────────────────────────────────────────

static char normalize_char(char c)
{
    if (c == '/') { return '\\';
}
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

static bool paths_equal(const std::string& a, const char* b)
{
    size_t len = a.size();
    for (size_t i = 0; i < len; i++) {
        if (b[i] == '\0') { return false;
}
        if (normalize_char(a[i]) != normalize_char(b[i])) { return false;
}
    }
    return b[len] == '\0';
}

static bool is_zlib_compressed(const uint8_t* data, size_t size)
{
    if (size < 2) { return false;
}
    // zlib header: CMF byte 0x78 = deflate with 32K window
    // FLG byte must satisfy (CMF * 256 + FLG) % 31 == 0
    // Common FLG values: 0x01, 0x5E, 0x9C, 0xDA
    if (data[0] != 0x78) { return false;
}
    uint16_t check = (static_cast<uint16_t>(data[0]) * 256) + data[1];
    return (check % 31) == 0;
}

// zlib's next_in is Bytef* (non-const), but inflate does not modify the input.
// Use memcpy to assign the pointer without const_cast.
static void set_zlib_input(z_stream& strm, const uint8_t* data, uint32_t size)
{
    std::memcpy(static_cast<void*>(&strm.next_in),
                static_cast<const void*>(&data), sizeof(data));
    strm.avail_in = size;
}

// ── Dat2Archive ──────────────────────────────────────────────────────────

Dat2Archive::Dat2Archive(Dat2Archive&& other) noexcept
    : m_file_buf(std::move(other.m_file_buf))
    , m_file_size(other.m_file_size)
    , m_data_end(other.m_data_end)
    , m_entries(std::move(other.m_entries))
{
    other.m_file_size = 0;
    other.m_data_end = 0;
}

Dat2Archive& Dat2Archive::operator=(Dat2Archive&& other) noexcept
{
    if (this != &other) {
        m_file_buf  = std::move(other.m_file_buf);
        m_file_size = other.m_file_size;
        m_data_end  = other.m_data_end;
        m_entries   = std::move(other.m_entries);
        other.m_file_size = 0;
        other.m_data_end  = 0;
    }
    return *this;
}

Dat2Result<Dat2Archive> Dat2Archive::open(const char* path)
{
    FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return Dat2Result<Dat2Archive>::fail(Dat2Error::FILE_OPEN_FAILED);
    }

    std::fseek(f, 0, SEEK_END);
    long file_len = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    if (file_len <= 0) {
        std::fclose(f);
        return Dat2Result<Dat2Archive>::fail(Dat2Error::FILE_TOO_SMALL);
    }

    size_t size = static_cast<size_t>(file_len);
    auto buf = std::make_unique<uint8_t[]>(size);

    if (std::fread(buf.get(), 1, size, f) != size) {
        std::fclose(f);
        return Dat2Result<Dat2Archive>::fail(Dat2Error::FILE_READ_FAILED);
    }
    std::fclose(f);

    return open_from_buffer(std::move(buf), size);
}

Dat2Result<Dat2Archive> Dat2Archive::open_from_buffer(std::unique_ptr<uint8_t[]> buf, size_t size)
{
    Dat2Archive archive;
    archive.m_file_buf  = std::move(buf);
    archive.m_file_size = size;

    Dat2Error err = archive.parse_tree();
    if (err != Dat2Error::OK) {
        return Dat2Result<Dat2Archive>::fail(err);
    }

    return Dat2Result<Dat2Archive>::success(std::move(archive));
}

uint32_t Dat2Archive::file_count() const
{
    return static_cast<uint32_t>(m_entries.size());
}

const std::vector<Dat2Entry>& Dat2Archive::entries() const
{
    return m_entries;
}

const Dat2Entry* Dat2Archive::find_entry(const char* path) const
{
    for (const auto& entry : m_entries) {
        if (paths_equal(entry.filename, path)) {
            return &entry;
        }
    }
    return nullptr;
}

// ── parse_tree ───────────────────────────────────────────────────────────
//
// DAT2 layout (from end of file):
//   [data section: concatenated file contents]
//   [num_files: u32]
//   [tree entries: variable-length]
//   [tree_size: u32]   ← includes its own 4 bytes in the count
//   [file_size: u32]   ← total file size
//
Dat2Error Dat2Archive::parse_tree()
{
    const uint8_t* buf = m_file_buf.get();
    const size_t   len = m_file_size;

    // Minimum: num_files(4) + tree_size(4) + file_size(4) = 12
    if (len < 12) {
        return Dat2Error::FILE_TOO_SMALL;
    }

    // Read file_size from last 4 bytes
    uint32_t file_size_field = 0;
    read_le_u32(buf, len, len - 4, file_size_field);
    if (static_cast<size_t>(file_size_field) != len) {
        return Dat2Error::FILE_SIZE_MISMATCH;
    }

    // Read tree_size (includes its own 4 bytes)
    uint32_t tree_size_raw = 0;
    read_le_u32(buf, len, len - 8, tree_size_raw);
    if (tree_size_raw < 4) {
        return Dat2Error::TREE_SIZE_INVALID;
    }

    // tree_size_raw includes the 4 bytes of the tree_size field itself
    uint32_t tree_entries_size = tree_size_raw - 4;

    // tree_end is right before tree_size field
    size_t tree_end = len - 8;
    if (tree_entries_size > tree_end) {
        return Dat2Error::TREE_SIZE_INVALID;
    }

    size_t tree_start = tree_end - tree_entries_size;

    // num_files is the 4 bytes before the tree entries
    if (tree_start < 4) {
        return Dat2Error::TREE_SIZE_INVALID;
    }

    uint32_t num_files = 0;
    read_le_u32(buf, len, tree_start - 4, num_files);

    // data section ends where num_files field starts
    m_data_end = tree_start - 4;

    // Parse tree entries
    m_entries.clear();
    m_entries.reserve(num_files);

    size_t pos = tree_start;
    for (uint32_t i = 0; i < num_files; i++) {
        // Each entry: filename_len(4) + filename(N) + is_compressed(1) + decompressed_size(4) + packed_size(4) + offset(4) = 17 + N
        const size_t ENTRY_HEADER = 4;
        const size_t ENTRY_FOOTER = 13; // 1 + 4 + 4 + 4

        if (pos + ENTRY_HEADER + ENTRY_FOOTER > tree_end) {
            return Dat2Error::TREE_PARSE_ERROR;
        }

        uint32_t filename_len = 0;
        read_le_u32(buf, len, pos, filename_len);
        pos += 4;

        size_t total_entry_remaining = filename_len + ENTRY_FOOTER;
        if (pos + total_entry_remaining > tree_end) {
            return Dat2Error::TREE_PARSE_ERROR;
        }

        std::string filename(buf + pos, buf + pos + filename_len);
        pos += filename_len;

        Dat2Entry entry;
        entry.filename = std::move(filename);

        uint8_t compressed_flag = 0;
        read_le_u8(buf, len, pos, compressed_flag);
        entry.is_compressed = (compressed_flag > 0);
        pos += 1;

        read_le_u32(buf, len, pos, entry.decompressed_size);
        pos += 4;

        read_le_u32(buf, len, pos, entry.packed_size);
        pos += 4;

        read_le_u32(buf, len, pos, entry.offset);
        pos += 4;

        m_entries.push_back(std::move(entry));
    }

    return Dat2Error::OK;
}

// ── extract ──────────────────────────────────────────────────────────────

Dat2Result<std::vector<uint8_t>> Dat2Archive::extract(const Dat2Entry& entry) const
{
    const uint8_t* buf = m_file_buf.get();

    // Bounds check: entry data must be within the data section
    size_t data_start = entry.offset;
    size_t data_end   = data_start + entry.packed_size;
    if (data_end > m_data_end || data_start > data_end) {
        return Dat2Result<std::vector<uint8_t>>::fail(Dat2Error::ENTRY_OUT_OF_BOUNDS);
    }

    const uint8_t* raw = buf + data_start;

    // Use zlib magic bytes as the primary compression check
    if (is_zlib_compressed(raw, entry.packed_size)) {
        std::vector<uint8_t> out(entry.decompressed_size);
        if (entry.decompressed_size == 0) {
            return Dat2Result<std::vector<uint8_t>>::success(std::move(out));
        }

        z_stream strm{};
        if (inflateInit(&strm) != Z_OK) {
            return Dat2Result<std::vector<uint8_t>>::fail(Dat2Error::DECOMPRESSION_FAILED);
        }

        set_zlib_input(strm, raw, entry.packed_size);
        strm.next_out  = out.data();
        strm.avail_out = entry.decompressed_size;

        int ret = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);

        if (ret != Z_STREAM_END) {
            return Dat2Result<std::vector<uint8_t>>::fail(Dat2Error::DECOMPRESSION_FAILED);
        }

        return Dat2Result<std::vector<uint8_t>>::success(std::move(out));
    }

    // Uncompressed: just copy
    std::vector<uint8_t> out(raw, raw + entry.packed_size);
    return Dat2Result<std::vector<uint8_t>>::success(std::move(out));
}

Dat2Status Dat2Archive::extract_to(const Dat2Entry& entry, uint8_t* out_buf, size_t buf_size) const
{
    const uint8_t* buf = m_file_buf.get();

    size_t data_start = entry.offset;
    size_t data_end   = data_start + entry.packed_size;
    if (data_end > m_data_end || data_start > data_end) {
        return { Dat2Error::ENTRY_OUT_OF_BOUNDS };
    }

    const uint8_t* raw = buf + data_start;

    if (is_zlib_compressed(raw, entry.packed_size)) {
        if (buf_size < entry.decompressed_size) {
            return { Dat2Error::ENTRY_OUT_OF_BOUNDS };
        }
        if (entry.decompressed_size == 0) {
            return { Dat2Error::OK };
        }

        z_stream strm{};
        if (inflateInit(&strm) != Z_OK) {
            return { Dat2Error::DECOMPRESSION_FAILED };
        }

        set_zlib_input(strm, raw, entry.packed_size);
        strm.next_out  = out_buf;
        strm.avail_out = static_cast<uInt>(buf_size);

        int ret = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);

        if (ret != Z_STREAM_END) {
            return { Dat2Error::DECOMPRESSION_FAILED };
        }

        return { Dat2Error::OK };
    }

    // Uncompressed
    if (buf_size < entry.packed_size) {
        return { Dat2Error::ENTRY_OUT_OF_BOUNDS };
    }
    std::memcpy(out_buf, raw, entry.packed_size);
    return { Dat2Error::OK };
}

} // namespace dat2
