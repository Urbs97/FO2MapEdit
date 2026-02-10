#include "dat2_tree_view.h"

#include "../Load_Files.h"
#include "../file_types/File_Type_Registry.h"
#include "../ui/ImGui_Warning.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

Dat2TreeNode build_dat2_tree(const std::vector<dat2::Dat2Entry>& entries) {
    Dat2TreeNode root;
    root.name = "/";

    for (const auto& entry : entries) {
        Dat2TreeNode* current = &root;
        const std::string& path = entry.filename;

        size_t start = 0;
        while (start < path.size()) {
            size_t sep = path.find('\\', start);
            if (sep == std::string::npos) {
                // final segment — this is a file (leaf node)
                std::string file_name = path.substr(start);
                Dat2TreeNode leaf;
                leaf.name = std::move(file_name);
                leaf.entry = &entry;
                current->children.push_back(std::move(leaf));
                break;
            }

            // intermediate segment — directory
            std::string dir_name = path.substr(start, sep - start);
            start = sep + 1;

            // find existing directory child
            Dat2TreeNode* found = nullptr;
            for (auto& child : current->children) {
                if (child.entry == nullptr && child.name == dir_name) {
                    found = &child;
                    break;
                }
            }

            if (found == nullptr) {
                Dat2TreeNode dir_node;
                dir_node.name = std::move(dir_name);
                current->children.push_back(std::move(dir_node));
                found = &current->children.back();
            }

            current = found;
        }
    }

    // Sort each node's children: directories first, then files, alphabetical within each group
    struct SortTree {
        static void sort(Dat2TreeNode& node) {
            std::sort(node.children.begin(), node.children.end(),
                      [](const Dat2TreeNode& a, const Dat2TreeNode& b) {
                          bool a_is_dir = (a.entry == nullptr);
                          bool b_is_dir = (b.entry == nullptr);
                          if (a_is_dir != b_is_dir) {
                              return a_is_dir; // directories first
                          }
                          return a.name < b.name;
                      });
            for (auto& child : node.children) {
                if (child.entry == nullptr) {
                    sort(child);
                }
            }
        }
    };
    SortTree::sort(root);

    return root;
}

const char* format_file_size(char* buf, int buf_size, uint32_t bytes) {
    if (bytes < 1024) {
        snprintf(buf, buf_size, "%u B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buf, buf_size, "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(buf, buf_size, "%.1f MB", bytes / (1024.0 * 1024.0));
    }
    return buf;
}

bool load_dat_archive(LF* F_Prop) {
    auto result = dat2::Dat2Archive::open(F_Prop->Opened_File);
    if (!result.ok()) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "[ERROR] load_dat_archive()\n\n"
                 "Failed to open DAT archive:\n%s\n\n%s",
                 F_Prop->Opened_File, dat2::dat2_error_str(result.error));
        set_popup_warning(msg);
        return false;
    }

    Dat2TreeNode tree = build_dat2_tree(result.value.entries());
    auto* info = new dat_info{std::move(result.value), std::move(tree)};

    F_Prop->dat = info;
    return true;
}

bool dat2_entry_is_previewable(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (dot == nullptr) {
        return false;
    }
    dot++; // skip the dot

    // uppercase copy for comparison
    char ext[8] = {};
    for (int i = 0; i < 7 && dot[i] != '\0'; i++) {
        ext[i] = static_cast<char>(toupper(static_cast<unsigned char>(dot[i])));
    }

    const FileTypeEntry* entry = find_file_type(ext);
    return (entry != nullptr) && (entry->flags & FileTypeFlags::HAS_PREVIEW);
}
