#pragma once

struct LF;
struct shader_info;
struct image_data;

bool open_DAT(LF* F_Prop, shader_info* shaders, image_data* img_data);
