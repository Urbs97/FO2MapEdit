#include "File_Type_Registry.h"

#include "../platform_io.h"
#include "open_DAT.h"
#include "open_FRM.h"
#include "open_Image.h"
#include "open_MSK.h"
#include "open_WMAP.h"

#include <cstdio>
#include <cstring>

static const FileTypeEntry s_file_types[] = {
    {{"FRM", "FR0", "FR1", "FR2", "FR3", "FR4", "FR5", nullptr},
     "Fallout FRM Sprite",
     img_type::FRM,
     FileTypeFlags::DRAG_DROP | FileTypeFlags::DIALOG | FileTypeFlags::HAS_IMAGE,
     open_FRM},

    {{"MSK", nullptr},
     "Fallout MSK Mask",
     img_type::MSK,
     FileTypeFlags::DRAG_DROP | FileTypeFlags::DIALOG | FileTypeFlags::HAS_IMAGE,
     open_MSK},

    {{"WMAP", nullptr},
     "Worldmap Project",
     img_type::UNK,
     FileTypeFlags::DRAG_DROP | FileTypeFlags::DIALOG | FileTypeFlags::HAS_IMAGE,
     open_WMAP},

    {{"DAT", nullptr},
     "Fallout DAT Archive",
     img_type::UNK,
     FileTypeFlags::DIALOG, // dialog only -- NOT drag-drop (preserves current behavior)
     open_DAT},

    {{"PNG", "JPG", "JPEG", "BMP", "GIF", nullptr},
     "Image File",
     img_type::OTHER,
     FileTypeFlags::DRAG_DROP | FileTypeFlags::DIALOG | FileTypeFlags::HAS_IMAGE,
     open_Image},
};

static constexpr int s_file_type_count = sizeof(s_file_types) / sizeof(s_file_types[0]);

const FileTypeEntry* find_file_type(const char* extension) {
    for (const auto& entry : s_file_types) {
        for (int j = 0; entry.extensions[j] != nullptr; j++) {
            if (io_strncmp(extension, entry.extensions[j],
                           static_cast<int>(strlen(entry.extensions[j]) + 1)) == 0) {
                return &entry;
            }
        }
    }
    return nullptr;
}

bool is_supported_format(const char* extension) {
    for (const auto& entry : s_file_types) {
        if (!(entry.flags & FileTypeFlags::DRAG_DROP)) {
            continue;
        }
        for (int j = 0; entry.extensions[j] != nullptr; j++) {
            if (io_strncmp(extension, entry.extensions[j],
                           static_cast<int>(strlen(entry.extensions[j]) + 1)) == 0) {
                return true;
            }
        }
    }
    return false;
}

int build_dialog_filter(char* buf, int buf_size) {
    // Build a filter string like:
    // "FRM/MSK/WMAP/DAT and image files(*.png;*.jpg;...){.fr0,.FR0,.fr1,.FR1,...}"
    int pos = 0;

    // Title part: "FRM/MSK/WMAP/DAT and image files"
    pos += snprintf(buf + pos, buf_size - pos, "FRM/MSK/WMAP/DAT and image files(");

    // Preview part: "*.ext;*.ext;..." (human-readable, lowercase)
    bool first_preview = true;
    for (const auto& entry : s_file_types) {
        if (!(entry.flags & FileTypeFlags::DIALOG)) {
            continue;
        }
        for (int j = 0; entry.extensions[j] != nullptr; j++) {
            if (!first_preview) {
                pos += snprintf(buf + pos, buf_size - pos, ";");
            }
            // Write lowercase version for the preview
            pos += snprintf(buf + pos, buf_size - pos, "*.");
            for (const char* c = entry.extensions[j]; *c != 0; c++) {
                char lower = static_cast<char>((*c >= 'A' && *c <= 'Z') ? (*c + 32) : *c);
                if (pos < buf_size - 1) {
                    buf[pos++] = lower;
                }
            }
            first_preview = false;
        }
    }
    // Special: add fr0-5 summary
    pos += snprintf(buf + pos, buf_size - pos, ")");

    // Filter part: "{.ext,.EXT,...}" (both cases for ImFileDialog)
    pos += snprintf(buf + pos, buf_size - pos, "{");
    bool first_filter = true;
    for (const auto& entry : s_file_types) {
        if (!(entry.flags & FileTypeFlags::DIALOG)) {
            continue;
        }
        for (int j = 0; entry.extensions[j] != nullptr; j++) {
            if (!first_filter) {
                pos += snprintf(buf + pos, buf_size - pos, ",");
            }
            // lowercase
            pos += snprintf(buf + pos, buf_size - pos, ".");
            for (const char* c = entry.extensions[j]; *c != 0; c++) {
                char lower = static_cast<char>((*c >= 'A' && *c <= 'Z') ? (*c + 32) : *c);
                if (pos < buf_size - 1) {
                    buf[pos++] = lower;
                }
            }
            // uppercase
            pos += snprintf(buf + pos, buf_size - pos, ",.");
            for (const char* c = entry.extensions[j]; *c != 0; c++) {
                if (pos < buf_size - 1) {
                    buf[pos++] = *c; // already uppercase in registry
                }
            }
            first_filter = false;
        }
    }
    pos += snprintf(buf + pos, buf_size - pos, "}");

    if (pos < buf_size) {
        buf[pos] = '\0';
    }
    return pos;
}
