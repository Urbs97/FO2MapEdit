#pragma once

#include "Load_Files.h"
#include "Load_Settings.h"

struct GLFWwindow;

extern user_info usr_info;
extern struct dropped_files all_dropped_files;

extern bool g_edit_mode_active;
extern bool g_reset_imgui_ini;
extern bool g_show_quit_confirm;
extern bool g_any_file_editing;
extern bool g_quit_after_save;
extern GLFWwindow* g_main_window;
