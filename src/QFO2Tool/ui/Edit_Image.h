#pragma once
// #include <SDL.h>
#include "../Image2Texture.h"
#include "../Stroke_State.h"

void Edit_Image(variables* My_Variables, ImVec2 img_pos, image_data* edit_data,
                image_data* img_data, ANM_Dir* edit_struct, int active_layer, bool Palette_Update,
                uint8_t* Color_Pick, StrokeState* stroke_state);
void draw_brush_cursor(StrokeState* stroke_state);
void draw_frame_boundary(image_data* edit_data, ImVec2 img_pos, int active_layer);
void draw_pixel_grid(image_data* edit_data, ImVec2 img_pos, int active_layer);
void brush_size_handler(variables* My_Variables);
ImVec2 display_img_ImGUI(variables* My_Variables, image_data* edit_data);

void init_edit_struct_ANM(ANM_Dir* edit_struct, image_data* edit_data, Palette* palette);
void commit_all_overlay_edits(image_data* edit_data);
void commit_map_edits(ANM_Dir* edit_struct, image_data* edit_data);
void draw_layer_panel(LF* F_Prop, shader_info* shaders, image_data* img_data);
