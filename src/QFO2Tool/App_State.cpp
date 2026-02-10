#include "App_State.h"

#include <GLFW/glfw3.h>

user_info usr_info;
struct dropped_files all_dropped_files = {0};

bool g_edit_mode_active = false;
bool g_reset_imgui_ini = false;
bool g_show_quit_confirm = false;
bool g_any_file_editing = false;
bool g_quit_after_save = false;
GLFWwindow* g_main_window = nullptr;
