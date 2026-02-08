#pragma once

#include "Load_Settings.h"
#include "MiniSDL.h"
#include "load_FRM_OpenGL.h"
#include "town_map_tiles.h"

enum Save_Type {
    single_frm,
    single_dir,
    all_dirs,
};

struct Save_Info {
    Save_Type s_type;
    int action_frame = 0;
};
void init_IFD();
bool save_FRM_SURFACE(char* save_name, image_data* img_data, user_info* usr_info,
                      Save_Info* sv_info, bool overwrite);
bool ImDialog_save_FRM_SURFACE(image_data* img_data, user_info* usr_info, Save_Info* sv_info);
bool ImDialog_save_TILE_SURFACE(image_data* img_data, user_info* usr_info, Save_Info* sv_info,
                                Surface* msk_srfc = nullptr, const char* preset_name = nullptr);
bool save_PNG_popup_INTERNAL(image_data* img_data, user_info* usr_info);
void save_as_GIF(image_data* img_data, struct user_info* usr_nfo);
void save_MSK_tile(uint8_t* texture_buffer, FILE* File_ptr, int width, int height);

uint8_t* blend_PAL_texture(image_data* img_data);

void Set_Default_Game_Path(user_info* user_info, char* exe_path);

tt_arr_handle* export_TMAP_tiles_POPUP(user_info* usr_info, Surface* srfc, Rect* offset,
                                       export_state* state);
void save_folder_dialog(user_info* usr);
