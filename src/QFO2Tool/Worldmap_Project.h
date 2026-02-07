#pragma once
#include "platform_io.h"
#include "MiniSDL.h"
#include "Load_Files.h"

// Fallout map tile size hardcoded in engine to 350x300 pixels WxH
#define WMAP_TILE_W (350)
#define WMAP_TILE_H (300)

struct wmap_info {
    int  version;
    char base_name[8];
    int  tiles_x;
    int  tiles_y;
    bool has_msk;
    char directory[MAX_PATH];
};

bool write_wmap_file(const char* base_path, const char* save_name,
                     int tiles_x, int tiles_y, bool has_msk);

bool parse_wmap_file(const char* wmap_path, wmap_info* info);

Surface* load_stitch_FRM_tiles(wmap_info* info, Palette* pal);
Surface* load_stitch_MSK_tiles(wmap_info* info);

bool load_wmap_project(const char* wmap_path, LF* F_Prop,
                       image_data* img_data, shader_info* shaders);
