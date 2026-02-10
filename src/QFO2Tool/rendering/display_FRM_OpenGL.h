#pragma once

#include "../load_FRM_OpenGL.h"
#include "shader_class.h"

mesh load_giant_triangle();
void animate_SURFACE_to_sub_texture(image_data* img_data, Surface* edit_srfc,
                                    uint64_t current_time);
void SURFACE_to_texture(Surface* src, GLuint texture, int width, int height, int alignment);
void PAL_SURFACE_to_sub_texture(uint8_t* pxls, GLuint texture, int x_offset, int y_offset,
                                int frm_width, int frm_height, int total_width, int total_height);
void draw_FRM_to_framebuffer(shader_info* shader_i, int width, int height, GLuint framebuffer,
                             GLuint texture);
void draw_PAL_to_framebuffer(Palette* pal, Shader* shader, mesh* triangle,
                             struct image_data* img_data, OverlayLayer* overlays,
                             int overlay_count);
void animate_OTHER_to_framebuff(Shader* shader, mesh* triangle, image_data* img_data,
                                uint64_t current_time);
void draw_texture_to_framebuffer(Palette* pal, Shader* shader, mesh* triangle, GLuint framebuffer,
                                 GLuint texture, int w, int h);
