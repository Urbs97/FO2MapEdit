#include "dat2_writer.h"

#include "dat2_io.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <zlib.h>

namespace dat2 {

namespace {
struct FileCloser {
    void operator()(FILE* f) const {
        if (f != nullptr) {
            std::fclose(f);
        }
    }
};
using FilePtr = std::unique_ptr<FILE, FileCloser>;
} // anonymous namespace

// ── write_archive ────────────────────────────────────────────────────────

Dat2Status write_archive(const char* output_path, const std::vector<Dat2WriteEntry>& entries,
                         const Dat2WriteOptions& options) {
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

    for (const auto& entry : entries) {
        EntryMeta meta;
        meta.filename = entry.archive_path;
        meta.offset = static_cast<uint32_t>(data_section.size());
        meta.decompressed_size = static_cast<uint32_t>(entry.data.size());

        if (options.compress && !entry.data.empty()) {
            uLongf compressed_size = compressBound(static_cast<uLong>(entry.data.size()));
            std::vector<uint8_t> compressed(compressed_size);
            int ret = compress2(compressed.data(), &compressed_size, entry.data.data(),
                                static_cast<uLong>(entry.data.size()), Z_DEFAULT_COMPRESSION);
            if (ret == Z_OK) {
                compressed.resize(compressed_size);
                meta.packed_size = static_cast<uint32_t>(compressed_size);
                meta.is_compressed = true;
                data_section.insert(data_section.end(), compressed.begin(), compressed.end());
            } else {
                return {Dat2Error::COMPRESSION_FAILED};
            }
        } else {
            meta.packed_size = meta.decompressed_size;
            meta.is_compressed = false;
            data_section.insert(data_section.end(), entry.data.begin(), entry.data.end());
        }

        metas.push_back(std::move(meta));
    }

    // Phase 2: build num_files field
    std::vector<uint8_t> num_files_bytes;
    write_le_u32(num_files_bytes, static_cast<uint32_t>(entries.size()));

    // Phase 3: build tree entries
    std::vector<uint8_t> tree_entries;
    for (const auto& meta : metas) {
        write_le_u32(tree_entries, static_cast<uint32_t>(meta.filename.size()));
        tree_entries.insert(tree_entries.end(), meta.filename.begin(), meta.filename.end());
        write_le_u8(tree_entries, meta.is_compressed ? 1 : 0);
        write_le_u32(tree_entries, meta.decompressed_size);
        write_le_u32(tree_entries, meta.packed_size);
        write_le_u32(tree_entries, meta.offset);
    }

    // Phase 4: tree_size = tree_entries.size() + 4 (includes itself)
    uint32_t tree_size = static_cast<uint32_t>(tree_entries.size()) + 4;

    // Phase 5: compute total file size
    uint32_t total_size =
        static_cast<uint32_t>(data_section.size() + 4 + tree_entries.size() + 4 + 4);

    // Phase 6: assemble the complete file
    std::vector<uint8_t> result;
    result.reserve(total_size);

    result.insert(result.end(), data_section.begin(), data_section.end());
    result.insert(result.end(), num_files_bytes.begin(), num_files_bytes.end());
    result.insert(result.end(), tree_entries.begin(), tree_entries.end());
    write_le_u32(result, tree_size);
    write_le_u32(result, total_size);

    // Phase 7: write to disk
    FilePtr f(std::fopen(output_path, "wb"));
    if (!f) {
        return {Dat2Error::FILE_WRITE_FAILED};
    }

    if (std::fwrite(result.data(), 1, result.size(), f.get()) != result.size()) {
        return {Dat2Error::FILE_WRITE_FAILED};
    }

    return {Dat2Error::OK};
}

// ── collect_from_directory ───────────────────────────────────────────────

Dat2Result<std::vector<Dat2WriteEntry>> collect_from_directory(const char* dir_path) {
    namespace fs = std::filesystem;

    std::vector<Dat2WriteEntry> entries;
    std::error_code ec;

    fs::path root(dir_path);
    if (!fs::is_directory(root, ec)) {
        return Dat2Result<std::vector<Dat2WriteEntry>>::fail(Dat2Error::DIRECTORY_READ_FAILED);
    }

    for (const auto& dir_entry : fs::recursive_directory_iterator(root, ec)) {
        if (ec) {
            return Dat2Result<std::vector<Dat2WriteEntry>>::fail(Dat2Error::DIRECTORY_READ_FAILED);
        }
        if (!dir_entry.is_regular_file()) {
            continue;
        }

        // Build archive path: relative to root, with backslash separators
        fs::path rel = fs::relative(dir_entry.path(), root, ec);
        if (ec) {
            return Dat2Result<std::vector<Dat2WriteEntry>>::fail(Dat2Error::DIRECTORY_READ_FAILED);
        }

        std::string archive_path = rel.u8string();
        std::replace(archive_path.begin(), archive_path.end(), '/', '\\');

        // Read file contents
        auto file_size = dir_entry.file_size(ec);
        if (ec) {
            return Dat2Result<std::vector<Dat2WriteEntry>>::fail(Dat2Error::FILE_READ_FAILED);
        }

        std::vector<uint8_t> data(static_cast<size_t>(file_size));
        if (file_size > 0) {
            FilePtr f(std::fopen(dir_entry.path().c_str(), "rb"));
            if (!f) {
                return Dat2Result<std::vector<Dat2WriteEntry>>::fail(Dat2Error::FILE_READ_FAILED);
            }
            if (std::fread(data.data(), 1, data.size(), f.get()) != data.size()) {
                return Dat2Result<std::vector<Dat2WriteEntry>>::fail(Dat2Error::FILE_READ_FAILED);
            }
        }

        entries.push_back({std::move(archive_path), std::move(data)});
    }

    // Check for iteration error after the loop
    if (ec) {
        return Dat2Result<std::vector<Dat2WriteEntry>>::fail(Dat2Error::DIRECTORY_READ_FAILED);
    }

    // Sort entries alphabetically for deterministic output
    std::sort(entries.begin(), entries.end(), [](const Dat2WriteEntry& a, const Dat2WriteEntry& b) {
        return a.archive_path < b.archive_path;
    });

    return Dat2Result<std::vector<Dat2WriteEntry>>::success(std::move(entries));
}

// ── collect_from_archive ─────────────────────────────────────────────────

Dat2Result<std::vector<Dat2WriteEntry>> collect_from_archive(const Dat2Archive& archive) {
    std::vector<Dat2WriteEntry> entries;
    entries.reserve(archive.file_count());

    for (const auto& src : archive.entries()) {
        auto extracted = archive.extract(src);
        if (!extracted.ok()) {
            return Dat2Result<std::vector<Dat2WriteEntry>>::fail(extracted.error);
        }

        entries.push_back({src.filename, std::move(extracted.value)});
    }

    return Dat2Result<std::vector<Dat2WriteEntry>>::success(std::move(entries));
}

} // namespace dat2
