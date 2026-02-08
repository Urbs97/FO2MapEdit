#pragma once

#include "MiniSDL.h"
#include "Palette_Cycle.h"

#include <cstdint>
uint8_t convert_colors(uint8_t bytes);

Palette* load_palette_from_path(const char* path);

Surface* PAL_Color_Convert(Surface* src, Palette* pal, int color_match_algo);

union Pxl_Err {
    struct {
        int r;
        int g;
        int b;
        int a;
    };
    int arr[4];
};

void Euclidian_Distance_Color_Match(Surface* Convert, Surface* Temp_Surface);
void clamp_dither(Surface* Surface_32, Pxl_Err* err, int pixel_idx, int factor);
void limit_dither(Surface* Surface_32, Pxl_Err* err, int x, int y);
