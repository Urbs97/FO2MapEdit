#pragma once
#define main app_man
#include "../src/msk2bmpGUI.cpp"

#include "../src/QFO2Tool/B_Endian.cpp"
#include "../src/QFO2Tool/display_FRM_OpenGL.cpp"
#include "../src/QFO2Tool/Edit_Animation.cpp"
#include "../src/QFO2Tool/Edit_Image.cpp"
#include "../src/QFO2Tool/Edit_TILES_LST.cpp"
#include "../src/QFO2Tool/FRM_Convert.cpp"
#include "../src/QFO2Tool/Image2Texture.cpp"
#include "../src/QFO2Tool/Image_Render.cpp"
#include "../src/QFO2Tool/ImGui_Warning.cpp"
#include "../src/QFO2Tool/Load_Animation.cpp"
#include "../src/QFO2Tool/Load_Files.cpp"
#include "../src/QFO2Tool/load_FRM_OpenGL.cpp"
#include "../src/QFO2Tool/Load_Settings.cpp"
#include "../src/QFO2Tool/MiniSDL.cpp"
#include "../src/QFO2Tool/MSK_Convert.cpp"
#include "../src/QFO2Tool/Palette_Cycle.cpp"
#include "../src/QFO2Tool/platform_io.cpp"
#include "../src/QFO2Tool/Preview_Image.cpp"
#include "../src/QFO2Tool/Preview_Tiles.cpp"
#include "../src/QFO2Tool/Save_Files.cpp"
#include "../src/QFO2Tool/shader_class.cpp"
#include "../src/QFO2Tool/timer_functions.cpp"
#include "../src/QFO2Tool/town_map_tiles.cpp"
#include "../src/QFO2Tool/Zoom_Pan.cpp"
#include "../src/QFO2Tool/Stroke_State.cpp"
#include "../src/QFO2Tool/tiles_pattern.cpp"
#include "../src/QFO2Tool/Proto_Files.cpp"
#include "../src/QFO2Tool/Worldmap_Project.cpp"
#undef main

#include <stdio.h>
#include <assert.h>

#define FAIL 0
#define PASS 1

#define TILE_W 80
#define TILE_H 36

struct data_ll {
    data_ll* next;
    uint8_t pxls[];
};