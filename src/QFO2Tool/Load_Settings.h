#pragma once

#include "platform_io.h"

#include <cstddef>

enum { MAX_KEY = 32, MAX_RECENT_FILES = 10 };

enum {
    CANCEL = 0,
    YES = 1,
    NO = 2,
};

struct fo2_files {
    char* FRM_TILES_LST = nullptr;
    char* PRO_TILES_LST = nullptr;
    char* PRO_TILE_MSG = nullptr;
    char* WORLDMAP_TXT = nullptr;
};

struct user_info {
    char default_save_path[MAX_PATH]{}; // change to last_save_path?
    char default_game_path[MAX_PATH]{};
    char default_load_path[MAX_PATH]{}; // change to last_load_path?
    char* exe_directory = nullptr;

    fo2_files game_files;

    bool show_image_stats{}; // TODO: remove, replace with window specific bool
    bool create_new_LST{};
    size_t length{};

    int recent_files_count = 0;
    char recent_files[MAX_RECENT_FILES][MAX_PATH]{};
};

enum img_type {
    UNK = -1,
    MSK = 0,
    FRM = 1,
    FR0 = 2,
    FRx = 3,
    TILE = 3,
    OTHER = 4,
};

void Load_Config(struct user_info* user_info, char* exe_path);
void write_cfg_file(struct user_info* user_info, char* exe_path);

void parse_data(char* file_data, size_t size, struct user_info* user_info);
void parse_key(const char* file_data, size_t size, struct config_data* config_data);
void parse_comment(const char* file_data, size_t size, struct config_data* config_data);
void parse_value(const char* file_data, size_t size, struct config_data* config_data,
                 struct user_info* user_info);
void store_config_info(struct config_data* config_data, struct user_info* user_info);

void add_recent_file(struct user_info* usr_info, const char* file_path);
void remove_recent_file(struct user_info* usr_info, int index);
