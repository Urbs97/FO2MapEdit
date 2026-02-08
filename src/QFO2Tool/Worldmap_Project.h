#pragma once

#include "Load_Files.h"
#include "MiniSDL.h"
#include "platform_io.h"

// Fallout map tile size hardcoded in engine to 350x300 pixels WxH
enum { WMAP_TILE_W = (350), WMAP_TILE_H = (300) };

#pragma pack(push, 1)
struct wmap_header {
    char magic[4];     // "WMAP"
    uint32_t version;  // 2
    char base_name[8]; // null-padded
    uint32_t tiles_x;
    uint32_t tiles_y;
    uint32_t flags;      // bit 0 = has_msk
    uint32_t frm_offset; // always 44 for v2
    uint32_t frm_size;
    uint32_t msk_offset; // 0 if no MSK
    uint32_t msk_size;
};
#pragma pack(pop)

struct wmap_info {
    int version;
    char base_name[64]; // project display name (not used for export)
    int tiles_x;
    int tiles_y;
    bool has_msk;
    char save_path[MAX_PATH];
};

enum {
    WRLDMP_ART_IDX = 339 // line index of wrldmp00.frm in vanilla intrface.lst (GOG/vanilla)
};

bool write_worldmap_txt(const char* output_path, const char* base_name, int tiles_x, int tiles_y,
                        Surface* msk_srfc);

bool save_wmap_project(const char* path, LF* F_Prop);

bool new_wmap_project(LF* F_Prop, image_data* img_data, shader_info* shaders, Surface* src,
                      const char* base_name, int tiles_x, int tiles_y);

bool load_wmap_project(const char* wmap_path, LF* F_Prop, image_data* img_data,
                       shader_info* shaders);

bool import_wmap_from_fo2(const char* data_path, const char* base_name, LF* F_Prop,
                          image_data* img_data, shader_info* shaders, int* out_msk_skipped);

bool resolve_path_icase(const char* base, const char* suffix, char* out, int out_size);
