#pragma once

#include "../Load_Settings.h"

#include <cstdint>

struct LF;
struct variables;
struct shader_info;
struct image_data;
struct user_info;

using FileOpenHandler = bool (*)(LF* F_Prop, shader_info* shaders, image_data* img_data);
using FileSavePopupHandler = bool (*)(LF* F_Prop, user_info* usr_info);
using FileQuickSaveHandler = bool (*)(LF* F_Prop, user_info* usr_info);

struct DrawContext {
    variables* vars;
    LF* F_Prop;
    user_info* usr_info;
    int counter; // window slot index (for unique popup IDs)
};

using FileToolbarHandler = void (*)(DrawContext* ctx); // per-type toolbar buttons
using FilePreviewHandler = void (*)(DrawContext* ctx); // per-type preview rendering

enum class FileTypeFlags : uint8_t {
    NONE = 0,
    DRAG_DROP = 1 << 0,     // included in Supported_Format() / drag-drop validation
    DIALOG = 1 << 1,        // included in ImFileDialog filter string
    HAS_IMAGE = 1 << 2,     // has ANM_dir data (run shared tail validation)
    HAS_EXPORT = 1 << 3,    // has export popup handler (interactive save-as)
    HAS_QUICKSAVE = 1 << 4, // has quick-save handler (non-interactive overwrite)
    HAS_TOOLBAR = 1 << 5,   // has toolbar drawing handler
    HAS_PREVIEW = 1 << 6,   // has preview drawing handler
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
    FileSavePopupHandler save_popup; // nullptr if no export
    FileQuickSaveHandler quick_save; // nullptr if no quick-save
    FileToolbarHandler toolbar;      // nullptr if no toolbar
    FilePreviewHandler preview;      // nullptr if no preview
};

const FileTypeEntry* find_file_type(const char* extension);
const FileTypeEntry* find_file_type_by_img_type(img_type type);
bool is_supported_format(const char* extension);
int build_dialog_filter(char* buf, int buf_size);
