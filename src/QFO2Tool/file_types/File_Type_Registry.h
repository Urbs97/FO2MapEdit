#pragma once

#include "../Load_Settings.h"

#include <cstdint>

struct LF;
struct shader_info;
struct image_data;

using FileOpenHandler = bool (*)(LF* F_Prop, shader_info* shaders, image_data* img_data);

enum class FileTypeFlags : uint8_t {
    NONE = 0,
    DRAG_DROP = 1 << 0, // included in Supported_Format() / drag-drop validation
    DIALOG = 1 << 1,    // included in ImFileDialog filter string
    HAS_IMAGE = 1 << 2, // has ANM_dir data (run shared tail validation)
};

inline FileTypeFlags operator|(FileTypeFlags a, FileTypeFlags b) {
    return static_cast<FileTypeFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool operator&(FileTypeFlags a, FileTypeFlags b) {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

struct FileTypeEntry {
    const char*
        extensions[8]; // null-terminated list, uppercase, no dot: {"FRM", "FR0", ..., nullptr}
    const char* label; // display name for dialog
    img_type type;     // the img_type this handler sets
    FileTypeFlags flags;
    FileOpenHandler open;
};

const FileTypeEntry* find_file_type(const char* extension);
bool is_supported_format(const char* extension);
int build_dialog_filter(char* buf, int buf_size);
