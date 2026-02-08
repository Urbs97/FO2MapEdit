#pragma once
#include "Load_Settings.h"
#include "load_FRM_OpenGL.h"
#include "shader_class.h"

#include <GLFW/glfw3.h>
#include <filesystem>
#include <glad/glad.h>

struct wmap_info;

// File info
struct LF {
    char Frst_File[MAX_PATH]{};
    char Prev_File[MAX_PATH]{};
    char Opened_File[MAX_PATH]{};
    char Next_File[MAX_PATH]{};
    char Last_File[MAX_PATH]{};

    char* c_name{};
    char* extension{};
    bool alpha = true;
    bool show_stats = false;
    bool show_squares = false;
    bool show_tiles = false;

    image_data img_data;
    image_data edit_data;

    bool file_open_window = false;
    bool preview_tiles_window = false;
    bool show_image_render = false;
    bool editing_enabled = false;
    bool palettized = false;

    int active_layer = -1; // -1 = map, 0..N-1 = overlay index

    wmap_info* wmap = nullptr;            // non-null = this slot is a worldmap project
    bool show_close_confirm = false;      // pending "unsaved changes" dialog for tab close
    bool pending_commit_and_save = false; // commit edit_struct → edit_data, then save
};

struct shader_info {
    Palette* FO_pal = nullptr;
    Shader* render_PAL_shader{};
    Shader* render_FRM_shader{};
    Shader* render_OTHER_shader{};
    mesh giant_triangle;
};

// wrapper for array of strings
struct dropped_files {
    size_t count;
    size_t total_size;
    // lay out null terminated strings one after another
    char* first_path;
};

struct image_paths {
    std::vector<std::filesystem::path> animation_images;
};

struct variables;

char* Program_Directory();
void dropped_files_callback(GLFWwindow* window, int count, const char** paths);

bool ImDialog_load_files(LF* F_Prop, image_data* img_data, user_info* usr_info, shader_info* shader,
                         LF* all_F_Prop, int open_count);

bool File_Type_Check(LF* F_Prop, shader_info* shaders, image_data* img_data, const char* file_name);
bool prep_extension(LF* F_Prop, user_info* usr_info, const char* file_name);
void Next_Prev_File(char* next, char* prev, char* frst, char* last, char* current);
void load_tile_texture(GLuint* texture, char* file_name);

bool drag_drop_POPUP(variables* My_Variables, LF* F_Prop, image_paths* images_arr, int* counter);
bool handle_directory_drop_POPUP(char* dir_name, image_paths* image_arr);

int find_open_file(LF* all_F_Prop, int open_count, const char* path);
void focus_file_window(int index);

void game_path_set_POPUP(user_info* usr_nfo);
void set_game_path_POPUP(user_info* usr_nfo);
void game_path_NOT_set_POPUP();
