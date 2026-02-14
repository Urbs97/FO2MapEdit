#pragma once

#include "dat2_archive.h"

#include <cstdint>
#include <string>
#include <vector>

struct LF; // forward declaration

struct Dat2TreeNode {
    std::string name;                       // directory or file name segment
    const dat2::Dat2Entry* entry = nullptr; // non-null for leaf (file) nodes
    std::vector<Dat2TreeNode> children;     // dirs first, then files, alphabetical
};

struct dat_info {
    dat2::Dat2Archive archive;                        // the parsed archive (move-constructed)
    Dat2TreeNode tree_root;                           // pre-built tree for ImGui rendering
    const dat2::Dat2Entry* pending_export = nullptr;  // entry waiting for save dialog
    const dat2::Dat2Entry* pending_export_ssl = nullptr; // entry waiting for SSL export
    const dat2::Dat2Entry* pending_preview = nullptr; // entry waiting to be previewed
    bool pending_repack = false; // repack button was clicked, waiting for save dialog
    bool repack_compress = true; // compress entries when repacking
    char search_filter[256] = {};
};

Dat2TreeNode build_dat2_tree(const std::vector<dat2::Dat2Entry>& entries);
const char* format_file_size(char* buf, int buf_size, uint32_t bytes);
bool load_dat_archive(LF* F_Prop);
bool dat2_entry_is_previewable(const char* filename);
