#pragma once

#include "dat2_archive.h"

#include <cstdint>
#include <string>
#include <vector>

namespace dat2 {

struct Dat2WriteEntry {
    std::string archive_path;  // backslash-separated path in archive
    std::vector<uint8_t> data; // uncompressed file contents
};

struct Dat2WriteOptions {
    bool compress = true; // zlib compress entries
};

// Write entries to a DAT2 archive on disk.
Dat2Status write_archive(const char* output_path, const std::vector<Dat2WriteEntry>& entries,
                         const Dat2WriteOptions& options);

// Recursively collect files from a directory into write entries.
// Archive paths use backslash separators relative to dir_path.
Dat2Result<std::vector<Dat2WriteEntry>> collect_from_directory(const char* dir_path);

// Extract all entries from an opened archive for repacking.
Dat2Result<std::vector<Dat2WriteEntry>> collect_from_archive(const Dat2Archive& archive);

} // namespace dat2
