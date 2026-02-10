#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <zlib.h>

#include "dat2/dat2_io.h"

struct TestFileEntry {
    std::string filename;       // backslash-separated path
    std::vector<uint8_t> data;  // uncompressed file contents
    bool compress = false;      // whether to zlib-compress in the DAT2
};

// Build a synthetic DAT2 archive in memory from the given entries.
// Returns a pair of (buffer, size).
inline std::pair<std::unique_ptr<uint8_t[]>, size_t>
build_test_dat2(const std::vector<TestFileEntry>& files)
{
    // Phase 1: build data section and collect metadata
    struct EntryMeta {
        std::string filename;
        uint32_t offset{};
        uint32_t packed_size{};
        uint32_t decompressed_size{};
        bool is_compressed{};
    };

    std::vector<uint8_t> data_section;
    std::vector<EntryMeta> metas;

    for (const auto& file : files) {
        EntryMeta meta;
        meta.filename = file.filename;
        meta.offset = static_cast<uint32_t>(data_section.size());
        meta.decompressed_size = static_cast<uint32_t>(file.data.size());

        if (file.compress && !file.data.empty()) {
            // Compress with zlib
            uLongf compressed_size = compressBound(static_cast<uLong>(file.data.size()));
            std::vector<uint8_t> compressed(compressed_size);
            int ret = compress2(compressed.data(), &compressed_size,
                                file.data.data(), static_cast<uLong>(file.data.size()),
                                Z_DEFAULT_COMPRESSION);
            if (ret == Z_OK) {
                compressed.resize(compressed_size);
                meta.packed_size = static_cast<uint32_t>(compressed_size);
                meta.is_compressed = true;
                data_section.insert(data_section.end(),
                                    compressed.begin(), compressed.end());
            } else {
                // Fallback to uncompressed
                meta.packed_size = meta.decompressed_size;
                meta.is_compressed = false;
                data_section.insert(data_section.end(),
                                    file.data.begin(), file.data.end());
            }
        } else {
            meta.packed_size = meta.decompressed_size;
            meta.is_compressed = false;
            data_section.insert(data_section.end(),
                                file.data.begin(), file.data.end());
        }

        metas.push_back(std::move(meta));
    }

    // Phase 2: build num_files field
    std::vector<uint8_t> num_files_bytes;
    dat2::write_le_u32(num_files_bytes, static_cast<uint32_t>(files.size()));

    // Phase 3: build tree entries
    std::vector<uint8_t> tree_entries;
    for (const auto& meta : metas) {
        // filename_len (u32)
        dat2::write_le_u32(tree_entries, static_cast<uint32_t>(meta.filename.size()));
        // filename bytes
        tree_entries.insert(tree_entries.end(),
                            meta.filename.begin(), meta.filename.end());
        // is_compressed (u8)
        tree_entries.push_back(meta.is_compressed ? 1 : 0);
        // decompressed_size (u32)
        dat2::write_le_u32(tree_entries, meta.decompressed_size);
        // packed_size (u32)
        dat2::write_le_u32(tree_entries, meta.packed_size);
        // offset (u32)
        dat2::write_le_u32(tree_entries, meta.offset);
    }

    // Phase 4: tree_size = tree_entries.size() + 4 (includes itself)
    uint32_t tree_size = static_cast<uint32_t>(tree_entries.size()) + 4;

    // Phase 5: compute total file size
    // data_section + num_files(4) + tree_entries + tree_size(4) + file_size(4)
    uint32_t total_size = static_cast<uint32_t>(
        data_section.size() + 4 + tree_entries.size() + 4 + 4
    );

    // Phase 6: assemble the complete file
    std::vector<uint8_t> result;
    result.reserve(total_size);

    // data section
    result.insert(result.end(), data_section.begin(), data_section.end());
    // num_files
    result.insert(result.end(), num_files_bytes.begin(), num_files_bytes.end());
    // tree entries
    result.insert(result.end(), tree_entries.begin(), tree_entries.end());
    // tree_size
    dat2::write_le_u32(result, tree_size);
    // file_size
    dat2::write_le_u32(result, total_size);

    auto buf = std::make_unique<uint8_t[]>(result.size());
    std::memcpy(buf.get(), result.data(), result.size());
    return { std::move(buf), result.size() };
}
