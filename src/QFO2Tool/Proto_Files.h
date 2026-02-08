#pragma once

#include "town_map_tiles.h"
#include "Load_Settings.h"

struct proto_info {
    char* name;
    char* description;
    int material_id;
    int pro_tile;
};

struct tile_proto {
    uint32_t ObjectID;
    uint32_t TextID;
    uint32_t FrmID;
    uint32_t Light_Radius;
    uint32_t Light_Intensity;
    uint32_t Flags;
    uint32_t MaterialID;
};

int get_material_id();
void export_PRO_tiles_POPUP(user_info* usr_nfo, tt_arr_handle* handle, export_state* state,
                            bool auto_export);
bool export_single_tile_PRO(char* game_path, tt_arr* tile, proto_info* info);
void export_tiles_POPUPS(export_state* state, char* FObuff);
bool load_PRO_tiles_LST(user_info* usr_nfo, export_state* state);
bool load_PRO_tiles_MSG(user_info* usr_nfo, export_state* state);
char* make_PRO_tiles_LST(tt_arr_handle* head, uint8_t* match_buff_src);
char* make_PRO_tile_MSG(proto_info* info, int tile_id);
char* append_PRO_tile_MSG_inplace(char* old_PRO_MSG, char* new_PRO_MSG, export_state* state);
char* check_PRO_LST_names(char* tiles_lst, tt_arr_handle* new_protos);
char* append_PRO_tiles_LST(char* old_PRO_LST, tt_arr_handle* head, export_state* state);
