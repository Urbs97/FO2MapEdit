#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace dat2 {

enum class Dat2Error : uint8_t {
    OK,
    FILE_OPEN_FAILED,
    FILE_READ_FAILED,
    FILE_TOO_SMALL,
    FILE_SIZE_MISMATCH,
    TREE_SIZE_INVALID,
    TREE_PARSE_ERROR,
    ENTRY_OUT_OF_BOUNDS,
    DECOMPRESSION_FAILED,
    FILE_WRITE_FAILED,
    COMPRESSION_FAILED,
    DIRECTORY_READ_FAILED,
};

const char* dat2_error_str(Dat2Error err);

template <typename T>
struct Dat2Result {
    Dat2Error error;
    T value;

    [[nodiscard]] bool ok() const { return error == Dat2Error::OK; }

    static Dat2Result success(T val)
    {
        return { Dat2Error::OK, std::move(val) };
    }

    static Dat2Result fail(Dat2Error err)
    {
        return { err, T{} };
    }
};

struct Dat2Status {
    Dat2Error error;
    [[nodiscard]] bool ok() const { return error == Dat2Error::OK; }
};

struct Dat2Entry {
    std::string filename;
    uint32_t offset = 0;
    uint32_t packed_size = 0;
    uint32_t decompressed_size = 0;
    bool is_compressed = false;
};

class Dat2Archive {
public:
    // Open and parse a DAT2 file from disk.
    static Dat2Result<Dat2Archive> open(const char* path);

    // Open and parse from a buffer already in memory (takes ownership).
    static Dat2Result<Dat2Archive> open_from_buffer(std::unique_ptr<uint8_t[]> buf, size_t size);

    Dat2Archive() = default;
    ~Dat2Archive() = default;

    // Move-only (owns file buffer)
    Dat2Archive(Dat2Archive&& other) noexcept;
    Dat2Archive& operator=(Dat2Archive&& other) noexcept;
    Dat2Archive(const Dat2Archive&) = delete;
    Dat2Archive& operator=(const Dat2Archive&) = delete;

    [[nodiscard]] uint32_t file_count() const;
    [[nodiscard]] const std::vector<Dat2Entry>& entries() const;

    // Case-insensitive search. Accepts both '/' and '\\' as separators.
    const Dat2Entry* find_entry(const char* path) const;

    // Extract entry data into a new vector.
    [[nodiscard]] Dat2Result<std::vector<uint8_t>> extract(const Dat2Entry& entry) const;

    // Extract entry data into a caller-provided buffer.
    Dat2Status extract_to(const Dat2Entry& entry, uint8_t* buf, size_t buf_size) const;

private:
    std::unique_ptr<uint8_t[]> m_file_buf;
    size_t m_file_size = 0;
    size_t m_data_end = 0; // end of the data section (start of num_files field)
    std::vector<Dat2Entry> m_entries;

    Dat2Error parse_tree();
};

} // namespace dat2
