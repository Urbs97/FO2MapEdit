// PLaces to post when I get this debugged and working well enough to post:
// https://www.nma-fallout.com/threads/help-with-fallout-map-rendering.201969/?ref=0xceed.com
// https://www.nma-fallout.com/threads/fallout-tile-geometry.160287/
// https://www.nma-fallout.com/threads/fallout-2-map-editor.194536/

// ImGUI File Dialogs
// https://github.com/aiekick/ImGuiFileDialog?tab=readme-ov-file

// #define _CRTDBG_MAP_ALLOC
////#define SDL_MAIN_HANDLED
//
// #define SET_CRT_DEBUG_FIELD(a) \
//    _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
// #define CLEAR_CRT_DEBUG_FIELD(a) \
//    _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

bool show_demo_window = false;

// Suppress known leaks in third-party libraries (fontconfig, GTK, libdecor,
// Wayland, EGL) so ASAN runs clean without needing LSAN_OPTIONS env var.
#ifdef __SANITIZE_ADDRESS__
extern "C" const char *__lsan_default_suppressions() {
    return "leak:libfontconfig.so\n"
           "leak:libpango-1.0.so\n"
           "leak:libpangoft2-1.0.so\n"
           "leak:libpangocairo-1.0.so\n"
           "leak:libgtk-3.so\n"
           "leak:libdecor-0.so\n"
           "leak:libdecor-gtk.so\n"
           "leak:libwayland-client.so\n"
           "leak:_glfwInitEGL\n"
           "leak:g_thread_proxy\n"
           "leak:libffi.so\n";
}
#endif

#include <glad/glad.h>

// ImGui header files
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>

#include <MiniSDL.h>

// TODO: fix this so it compiles for windows
#ifdef QFO2_WINDOWS
#include <Windows.h>
#endif

// My header files
#include "Edit_Image.h"
#include "FRM_Convert.h"
#include "Image2Texture.h"
#include "Load_Files.h"
#include "Load_Settings.h"
#include "Preview_Image.h"
#include "Save_Files.h"
#include "platform_io.h"

#include "Edit_Animation.h"
#include "MSK_Convert.h"
#include "Palette_Cycle.h"
#include "Preview_Tiles.h"
#include "Stroke_State.h"
#include "display_FRM_OpenGL.h"

#include "City_Layer.h"
#include "ImGui_Warning.h"
#include "Zoom_Pan.h"
#include "Worldmap_Project.h"
#include "timer_functions.h"

#include <ImFileDialog.h>

// remove

// Our state
user_info usr_info;
static struct dropped_files all_dropped_files = {0};

// File->New Worldmap Project state
static bool g_new_wmap_pending = false;
static Surface *g_new_wmap_source = nullptr;
static char g_new_wmap_base_name[64] = "WRLDMP";
static int g_new_wmap_tiles_x = 0;
static int g_new_wmap_tiles_y = 0;

// File->Import Worldmap from FO2 state
static bool g_import_wmap_pending = false;
static char g_import_wmap_data_path[MAX_PATH] = "";
static char g_import_wmap_base_name[64] = "WRLDMP";
static char g_import_error[2048] = "";
static bool g_import_wmap_done = false;
static bool g_import_error_pending = false;

// Edit state for Show_Preview_Window (file-scope so shutdown can clean up)
static ANM_Dir g_edit_struct[6];
static StrokeState g_stroke_state;
static LF *g_edit_state_owner = nullptr;

// Function declarations
void Show_Preview_Window(variables *My_Variables, LF *F_Prop, int counter);
void Preview_Tiles_Window(variables *My_Variables, LF *F_Prop, int counter);
void Show_Image_Render(variables *My_Variables, LF *F_Prop,
                       struct user_info *usr_info, int counter);

void Show_Palette_Window(struct variables *My_Variables, bool show_msk_palette);
void Show_City_Info_Window(struct variables *My_Variables);

static void ShowMainMenuBar(int *counter, struct variables *My_Variables);
void Open_Files(struct user_info *usr_info, int *counter, Palette *pxlFMT,
                struct variables *My_Variables);

void main_window_bttns(variables *My_Variables, int *counter);
bool save_FRM_popup(LF *F_Prop);
bool save_MSK_popup(LF *F_Prop);
bool save_TILE_popup(LF *F_Prop);



static void glfw_error_callback(int error, const char *description) {
  // Suppress GLFW_FEATURE_UNAVAILABLE (65548) for Wayland window position
  // warnings
  if (error == 65548) {
    return;
}
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Set to true when edit mode is active — prevents Escape from closing the app
static bool g_edit_mode_active = false;
static bool g_reset_imgui_ini = false;

// Unsaved-edits confirmation state
static bool g_show_quit_confirm = false;
static bool g_any_file_editing = false;
static bool g_quit_after_save = false;
static GLFWwindow *g_main_window = nullptr;

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && !g_edit_mode_active) {
    if (g_any_file_editing) {
      g_show_quit_confirm = true;
    } else {
      glfwSetWindowShouldClose(window, 1);
    }
  }
}

void window_close_callback(GLFWwindow *window) {
  if (g_any_file_editing) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    g_show_quit_confirm = true;
  }
}

// Main code
int main(int argc, char **argv) {
  // AllocConsole();       //to turn on console from code
  // FILE* fDummy;
  // freopen_s(&fDummy, "CONIN$",  "r", stdin);
  // freopen_s(&fDummy, "CONOUT$", "w", stderr);
  // freopen_s(&fDummy, "CONOUT$", "w", stdout);
  // FreeConsole();        //used to disable the console while rendering (maybe
  // attach to a button?)
  int my_argc = 0;

#ifdef QFO2_WINDOWS
  LPWSTR *my_argv = CommandLineToArgvW(GetCommandLineW(), &my_argc);
#elif defined(QFO2_LINUX)
  char **my_argv = argv;
  my_argc = argc;
#endif

  glfwSetErrorCallback(glfw_error_callback);
  if (glfwInit() == 0) {
    printf("\nglfwInit() failed\n\n");
    return 1;
  }

  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 330 core";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only

  // Create window with graphics context
  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "FO2MapEdit", nullptr, nullptr);
  if (window == nullptr) {
    return 1;
  }

  g_main_window = window;
  glfwSetKeyCallback(window, key_callback);
  glfwSetWindowCloseCallback(window, window_close_callback);
  glfwSetDropCallback(window, dropped_files_callback);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
    printf("Glad Loader failed?...");
    exit(-1);
  } else {
    printf("Vendor: %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  }

  // State variables
  struct variables My_Variables = {};
  My_Variables.exe_directory = Program_Directory();
  usr_info.exe_directory = My_Variables.exe_directory;

  char vbuffer[MAX_PATH];
  char fbuffer[MAX_PATH];
  snprintf(vbuffer, sizeof(vbuffer), "%s%s", My_Variables.exe_directory,
           "resources/shaders/passthru_shader.vert");
  snprintf(fbuffer, sizeof(fbuffer), "%s%s", My_Variables.exe_directory,
           "resources/shaders/render_PAL.frag");
  My_Variables.shaders.render_PAL_shader = new Shader(vbuffer, fbuffer);

  snprintf(vbuffer, sizeof(vbuffer), "%s%s", My_Variables.exe_directory,
           "resources/shaders/passthru_shader.vert");
  snprintf(fbuffer, sizeof(fbuffer), "%s%s", My_Variables.exe_directory,
           "resources/shaders/render_FRM.frag");
  My_Variables.shaders.render_FRM_shader = new Shader(vbuffer, fbuffer);

  snprintf(vbuffer, sizeof(vbuffer), "%s%s", My_Variables.exe_directory,
           "resources/shaders/passthru_shader.vert");
  snprintf(fbuffer, sizeof(fbuffer), "%s%s", My_Variables.exe_directory,
           "resources/shaders/passthru_shader.frag");
  My_Variables.shaders.render_OTHER_shader = new Shader(vbuffer, fbuffer);

  snprintf(vbuffer, sizeof(vbuffer), "%s%s", My_Variables.exe_directory,
           "resources/grid-texture.png");
  // TODO: am I using tile_texture_prev or tile_texture_rend anymore?
  load_tile_texture(&My_Variables.tile_texture_prev, vbuffer);
  snprintf(vbuffer, sizeof(vbuffer), "%s%s", My_Variables.exe_directory,
           "resources/blue_tile_mask.png");
  load_tile_texture(&My_Variables.tile_texture_rend, vbuffer);

  // TODO: add user input for a user-specified palette
  snprintf(vbuffer, sizeof(vbuffer), "%s%s", My_Variables.exe_directory,
           "resources/palette/fo_color.pal");
  My_Variables.FO_Palette = load_palette_from_path(vbuffer);
  My_Variables.FO_Palette->num_colors =
      228; // TODO: delete? change so palette size is passed into load call

  Load_Config(&usr_info, My_Variables.exe_directory);

  My_Variables.shaders.FO_pal = My_Variables.FO_Palette;

  My_Variables.shaders.giant_triangle = load_giant_triangle();

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad
  // Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport /
                                                      // Platform Windows
  // io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;

  snprintf(vbuffer, sizeof(vbuffer), "%s%s", My_Variables.exe_directory,
           "imgui.ini");
  // io.IniFilename = vbuffer;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();
  if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
    style.WindowRounding = 0.0F;
    style.Colors[ImGuiCol_WindowBg].w = 1.0F;
  }

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can
  // also load multiple fonts and use ImGui::PushFont()/PopFont() to select
  // them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you
  // need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please
  // handle those errors in your application (e.g. use an assertion, or display
  // an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored
  // into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which
  // ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string
  // literal you need to write a double backslash \\ !

  snprintf(vbuffer, sizeof(vbuffer), "%s%s", My_Variables.exe_directory,
           "resources//fonts//OpenSans-Bold.ttf");
  io.Fonts->AddFontDefault();
  My_Variables.Font =
      io.Fonts->AddFontFromFileTTF(vbuffer, static_cast<float>(My_Variables.global_font_size));

  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  // ImFont* font =
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
  // NULL, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

  // this counter is used to identify which image slot is being used for now
  // TODO: need to swap this for a linked list (or a static F_Prop?),
  //       or store current image slot in the window itself
  static int counter = 0;

#ifdef QFO2_WINDOWS
  if (my_argv == NULL) {
    MessageBox(NULL, L"Something went wrong?", L"argv is NULL?",
               MB_ABORTRETRYIGNORE);
  } else {
    if (my_argc > 1) {
      LF *F_Prop = &My_Variables.F_Prop[counter];
      F_Prop->file_open_window =
          File_Type_Check(F_Prop, &My_Variables.shaders, &F_Prop->img_data,
                          io_wchar_utf8(my_argv[1]));
      if (F_Prop->file_open_window) {
        counter++;
      }
    }
  }
#elif defined(QFO2_LINUX)
  if (my_argc > 1) {
    // TODO: this currently requires full path from my_argv,
    //       need to somehow implement relative pathing to load relative files
    //       (for instance, in the same folder)
    // TODO: actually, I should probably expand this to parse the string for
    //       automated stuff
    LF *F_Prop = &My_Variables.F_Prop[counter];
    F_Prop->file_open_window = File_Type_Check(F_Prop, &My_Variables.shaders,
                                               &F_Prop->img_data, my_argv[1]);
    if (F_Prop->file_open_window) {
      counter++;
    }
  }
#endif

  // Our state
  ImVec4 clear_color = ImVec4(0.45F, 0.55F, 0.60F, 1.00F);
  // used to reset the default layout back to original
  bool firstframe = true;

  // Main loop
  while (glfwWindowShouldClose(window) == 0) {
    // handling dropped files in different windows
    bool file_drop_frame = all_dropped_files.count > 0;

    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application. Generally you may always pass all inputs
    // to dear imgui, and hide them from your application based on those two
    // flags.

    glfwPollEvents();

    { // mouse position handling for panning
      // store previous mouse position before assigning current
      ImVec2 old_mouse_pos = My_Variables.new_mouse_pos;

      // store current mouse position
      My_Variables.new_mouse_pos.x = ImGui::GetMousePos().x;
      My_Variables.new_mouse_pos.y = ImGui::GetMousePos().y;

      // store offset for mouse movement between frames
      My_Variables.mouse_delta.x =
          My_Variables.new_mouse_pos.x - old_mouse_pos.x;
      My_Variables.mouse_delta.y =
          My_Variables.new_mouse_pos.y - old_mouse_pos.y;
    }

    // Store these variables at frame start for cycling palette colors and
    // animations
    My_Variables.CurrentTime_ms = nano_time() / 1'000'000;
    My_Variables.Palette_Update = false;

    // end of event
    // handling/////////////////////////////////////////////////////////////////////////

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window
    //    (Most of the sample code is in ImGui::ShowDemoWindow()!
    //    You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window) {
      ImGui::ShowDemoWindow(&show_demo_window);
}

    ImGuiID dockspace_id =
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    ImGuiID dock_id_right = 0;
    ShowMainMenuBar(&counter, &My_Variables);

    if (firstframe) {
      firstframe = false;
      ImGui::DockBuilderAddNode(dockspace_id,
                                ImGuiDockNodeFlags_DockSpace); // Add empty node
      ImGui::DockBuilderSetNodeSize(dockspace_id,
                                    ImGui::GetMainViewport()->WorkSize);

      ImGuiID dock_main_id =
          dockspace_id; // This variable will track the docking node.
      ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(
          dock_main_id, ImGuiDir_Left, 0.35F, nullptr, &dock_main_id);
      ImGuiID dock_id_bleft = ImGui::DockBuilderSplitNode(
          dock_id_left, ImGuiDir_Down, 0.64F, nullptr, &dock_id_left);
      ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(
          dock_main_id, ImGuiDir_Right, 0.50F, nullptr, &dock_main_id);

      ImGui::DockBuilderDockWindow("###file", dock_id_left);
      ImGui::DockBuilderDockWindow("###palette", dock_id_bleft);
      ImGui::DockBuilderDockWindow("###preview00", dock_main_id);
      ImGui::DockBuilderDockWindow("###render00", dock_id_right);

      for (int i = 1; i <= 99; i++) {
        char buff1[13];
        sprintf(buff1, "###preview%02d", i);
        char buff2[12];
        sprintf(buff2, "###render%02d", i);

        ImGui::DockBuilderDockWindow(buff1, dock_main_id);
        ImGui::DockBuilderDockWindow(buff2, dock_id_right);
      }
      ImGui::DockBuilderFinish(dockspace_id);
    }

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair
    // to create a named window.
    ImGui::Begin("File Info###file"); // Create a window and append into it.

    // load files
    // used this to create an frm from palette LUT
    //  if (ImGui::Button("Save palette animation...")) {
    //      char path_buffer[MAX_PATH];
    //      snprintf(path_buffer, sizeof(path_buffer), "%s%s",
    //      usr_info.exe_directory, "/resources/palette/fo_color.pal");
    //      //file management
    //      uint8_t* palette_animation = (uint8_t*)malloc(1024*32);
    //      FILE *File_ptr = fopen(path_buffer, "rb");
    //      fseek(File_ptr, 768, SEEK_SET);
    //      fread(palette_animation, 1024*32, 1, File_ptr);
    //      fclose(File_ptr);
    //      FRM_Header header;
    //      header.version = 4;
    //      header.FPS = 10;
    //      header.Frames_Per_Orient = 32;
    //      header.Frame_Area = 32*32 + sizeof(FRM_Frame);
    //      B_Endian::flip_header_endian(&header);
    //      FRM_Frame frame;
    //      frame.Frame_Height = 32;
    //      frame.Frame_Width  = 32;
    //      frame.Frame_Size   = 32*32;
    //      B_Endian::flip_frame_endian(&frame);
    //      uint8_t* ptr = palette_animation;
    //      snprintf(path_buffer, sizeof(path_buffer), "%s%s",
    //      usr_info.exe_directory, "/resources/palette/palette_animation.FRM");
    //      FILE* file = fopen(path_buffer, "wb");
    //      fwrite(&header, sizeof(FRM_Header), 1, file);
    //      for (int i = 0; i < 32; i++)
    //      {
    //          fwrite(&frame, sizeof(FRM_Frame), 1, file);
    //          fwrite(ptr, 1024, 1, file);
    //          ptr += 1024;
    //      }
    //      fclose(file);
    //      free(palette_animation);
    //  }

    ImGui::SameLine();
    ImGui::Text("Number Windows = %d", counter);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0F / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

#pragma region buttons
    main_window_bttns(&My_Variables, &counter);

    // set contextual menu for main window
    // when file is dropped on window
    if (ImGui::IsWindowHovered() && file_drop_frame) {
      My_Variables.window_number_focus = -1;
      My_Variables.edit_image_focused = false;
    }

    static image_paths images_arr[6];
    bool does_window_exist = (My_Variables.window_number_focus > -1);
    int num = does_window_exist ? My_Variables.window_number_focus : counter;
    // popup for handling drag and drop animation sequences
    bool clear_images_arr = drag_drop_POPUP(
        &My_Variables, &My_Variables.F_Prop[num], images_arr, &counter);

    if (clear_images_arr) {
      for (auto & i : images_arr) {
        i.animation_images.clear();
      }
    }

    // handle opening dropped files
    if (file_drop_frame) {
      char *path = all_dropped_files.first_path;

      for (int i = 0; i < all_dropped_files.count; i++) {
        bool is_directory = handle_directory_drop_POPUP(path, images_arr);
        if (!is_directory) {
          int existing = find_open_file(My_Variables.F_Prop, counter, path);
          if (existing >= 0) {
            focus_file_window(existing);
            add_recent_file(&usr_info, path);
          } else {
            LF *F_Prop = &My_Variables.F_Prop[counter];

            F_Prop->file_open_window = File_Type_Check(
                F_Prop, &My_Variables.shaders, &F_Prop->img_data, path);
            if (F_Prop->file_open_window) {
              add_recent_file(&usr_info, path);
              counter++;
            }
          }
        }
        path += strlen(path) + 1;
      }

      free(all_dropped_files.first_path);
      memset(&all_dropped_files, 0, sizeof(dropped_files));
    }

    show_popup_warnings();

    ImGui::End();

    // contextual palette window for MSK vs FRM editing
    // Show MSK palette only when editing a paint-based overlay (not interactive layers like City)
    {
      bool show_msk_palette = false;
      if (My_Variables.window_number_focus > -1) {
        LF* focused = &My_Variables.F_Prop[My_Variables.window_number_focus];
        int al = focused->active_layer;
        if (al >= 0 && al < focused->img_data.overlay_count &&
            !focused->img_data.overlay[al].interactive) {
          show_msk_palette = true;
        }
      }
      Show_Palette_Window(&My_Variables, show_msk_palette);
      Show_City_Info_Window(&My_Variables);
    }

    // update palette at regular intervals
    My_Variables.Palette_Update = update_PAL_array(My_Variables.shaders.FO_pal,
                                                   static_cast<double>(My_Variables.CurrentTime_ms));

    for (int i = 0; i < counter; i++) {
      if (My_Variables.F_Prop[i].file_open_window) {
        Show_Preview_Window(&My_Variables, &My_Variables.F_Prop[i], i);
      }
    }

    // Update global edit mode flag so Escape key doesn't close app during
    // editing
    g_edit_mode_active = My_Variables.edit_image_focused;

    // Track whether any open file has unsaved edits
    g_any_file_editing = false;
    for (int i = 0; i < counter; i++) {
      if (My_Variables.F_Prop[i].file_open_window &&
          My_Variables.F_Prop[i].dirty) {
        g_any_file_editing = true;
        break;
      }
    }

    // Two-phase quit: after "Save & Quit" disables editing, wait for cleanup
    // then close
    if (g_quit_after_save && !g_any_file_editing) {
      glfwSetWindowShouldClose(g_main_window, GLFW_TRUE);
      g_quit_after_save = false;
    }

    // Quit confirmation dialog when files have unsaved edits
    if (g_show_quit_confirm) {
      ImGui::OpenPopup("Unsaved Changes##quit");
      g_show_quit_confirm = false;
    }
    if (ImGui::BeginPopupModal("Unsaved Changes##quit", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("There are files with unsaved edits.");
      ImGui::Separator();
      if (ImGui::Button("Save & Quit")) {
        for (int i = 0; i < counter; i++) {
          LF *fp = &My_Variables.F_Prop[i];
          if (!fp->file_open_window || !fp->dirty) {
            continue;
}
          fp->pending_commit_and_save = true;
          fp->editing_enabled = false;
        }
        g_quit_after_save = true;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Quit without saving")) {
        ImGui::CloseCurrentPopup();
        glfwSetWindowShouldClose(g_main_window, GLFW_TRUE);
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    // glUseProgram(0); // You may want this if using this code in an OpenGL 3+
    // context where shaders may be bound
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we
    // save/restore it to make it easier to paste this code elsewhere.
    if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
      GLFWwindow *current_context_backup = glfwGetCurrentContext();

      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(current_context_backup);
    }
    glfwSwapBuffers(window);
  }

  // Cleanup
  // TODO: test if freeing manually vs freeing by hand? is faster/same
  const char *ini_path = ImGui::GetIO().IniFilename;
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  if (g_reset_imgui_ini && (ini_path != nullptr)) {
    std::remove(ini_path);
  }

  stroke_state_cleanup(&g_stroke_state);

  for (auto & i : My_Variables.F_Prop) {
    Clear_img_data(&i.img_data);
    Clear_img_data(&i.edit_data);
    free(i.wmap);
    i.wmap = nullptr;
  }

  delete My_Variables.shaders.render_PAL_shader;
  delete My_Variables.shaders.render_FRM_shader;
  delete My_Variables.shaders.render_OTHER_shader;
  free(My_Variables.FO_Palette);

  glfwDestroyWindow(window);
  glfwTerminate();

  // write config file when closing
  write_cfg_file(&usr_info, usr_info.exe_directory);

#ifdef QFO2_WINDOWS
  // LocalFree(my_argv);
#endif

  return 0;
}
// end of
// main////////////////////////////////////////////////////////////////////////

// copies dropped file-paths to
// global dropped_files* all_dropped_files
void dropped_files_callback(GLFWwindow *window, int count, const char **paths) {
  if (count <= 0) { return;
}
  size_t size = 0;
  // get total length of all strings
  for (int i = 0; i < count; i++) {
    size += strlen(paths[i]) + 1;
  }

  // all_dropped_files is global
  char *c = nullptr;
  if (all_dropped_files.count > 0) {
    // if already storing filenames
    c = (char *)realloc(all_dropped_files.first_path,
                        all_dropped_files.total_size + size);
    all_dropped_files.first_path = c;
    c += size;
  } else {
    c = (char *)malloc(size);
    all_dropped_files.first_path = c;
  }

  all_dropped_files.count += count;
  all_dropped_files.total_size += size;
  for (int i = 0; i < count; i++) {
    size_t len = strlen(paths[i]) + 1;
    memcpy(c, paths[i], len);
    c += len;
  }
}

// TODO: need to add direct MSK file editing
//       probably in a different function?
void init_edit_struct_ANM(ANM_Dir *edit_struct, image_data *edit_data,
                          Palette *palette) {
  // this is for editing MSK files when loading them solo
  if (edit_data->ANM_dir == nullptr) {
    // edit_data->display_orient_num = 0;
    // edit_data->FRM_hdr
    edit_struct[0].frame_data = (Surface **)malloc(sizeof(Surface *));
    if (edit_struct[0].frame_data == nullptr) {
      // TODO: log out to txt file
      set_popup_warning("[ERROR] init_edit_struct_ANM()\n\n"
                        "Unable to allocate memory for edit_frame.\n");
      printf("Unable to allocate memory for edit_frame: %d\n", __LINE__);
      return;
    }
    edit_struct[0].frame_data[0] =
        Create_8Bit_Surface(edit_data->width, edit_data->height, palette);
    if (edit_struct[0].frame_data[0] == nullptr) {
      free(static_cast<void*>(edit_struct[0].frame_data));
      // TODO: log out to txt file
      set_popup_warning("[ERROR] init_edit_struct_ANM()\n\n"
                        "Unable to create 8bit surface.\n");
      printf("Unable to create 8bit surface: %d\n", __LINE__);
      return;
    }
    edit_data->ANM_dir = (ANM_Dir *)malloc(sizeof(ANM_Dir));
    if (edit_data->ANM_dir == nullptr) {
      free(static_cast<void*>(edit_struct[0].frame_data));
      FreeSurface(edit_struct[0].frame_data[0]);
      // TODO: log out to txt file
      set_popup_warning("[ERROR] init_edit_struct_ANM()\n\n"
                        "Unable to create 8bit surface.\n");
      printf("Unable to create 8bit surface: %d\n", __LINE__);
      return;
    }
    edit_data->ANM_dir[0].orientation = Direction::NE;
    edit_data->save_ptr = edit_struct;
    return;
  }

  for (int dir = 0; dir < 6; dir++) {
    int num_frames = edit_data->ANM_dir[dir].num_frames;
    edit_struct[dir].frame_data =
        (Surface **)malloc(num_frames * sizeof(Surface *));

    for (int frame = 0; frame < num_frames; frame++) {
      if (edit_data->ANM_dir[dir].frame_data == nullptr) {
        break;
      }

      // TODO: maybe this needs to be "edit_data->FRM_dir[0].bounding_box.x1"
      // etc?
      //       doing this might make it easier to edit a frame (maybe fewer
      //       crashes?) but doing this and painting outside the official
      //       Frame_Width/_Height would have to be dealt with by expanding the
      //       _Width/_Height whenever this happens AND give the user some
      //       feedback that this is happening
      Surface *src = edit_data->ANM_dir[dir].frame_data[frame];
      Surface *dst = Create_8Bit_Surface(src->w, src->h, palette);

      memcpy(dst->pxls, src->pxls, static_cast<size_t>(src->w) * src->h);

      edit_struct[dir].frame_data[frame] = dst;
    }
  }
  edit_data->save_ptr = edit_struct;
}

void commit_all_overlay_edits(image_data *edit_data) {
  for (int i = 0; i < edit_data->overlay_count; i++) {
    commit_layer_edits(&edit_data->overlay[i]);
  }
}

void commit_map_edits(ANM_Dir *edit_struct, image_data *edit_data) {
  if ((edit_struct == nullptr) || (edit_data->ANM_dir == nullptr)) {
    return;
}
  for (int dir = 0; dir < 6; dir++) {
    int num_frames = edit_data->ANM_dir[dir].num_frames;
    if (edit_struct[dir].frame_data == nullptr) {
      continue;
}
    for (int frame = 0; frame < num_frames; frame++) {
      Surface *src = edit_struct[dir].frame_data[frame];
      Surface *dst = edit_data->ANM_dir[dir].frame_data[frame];
      if ((src == nullptr) || (dst == nullptr)) {
        continue;
}
      if (src->w != dst->w || src->h != dst->h) {
        continue;
}
      memcpy(dst->pxls, src->pxls, static_cast<size_t>(src->w) * src->h);
    }
  }
}

// Layer panel — allows switching between Map and overlay layers
void draw_layer_panel(LF *F_Prop, shader_info *shaders, image_data *img_data) {
  ImGui::Separator();
  ImGui::Text("Layers");

  // Map layer (always shown)
  {
    bool selected = (F_Prop->active_layer == -1);
    if (ImGui::Selectable("  Map", selected)) {
      // Commit overlay working buffer before switching away
      if (F_Prop->active_layer >= 0 && F_Prop->active_layer < img_data->overlay_count) {
        commit_layer_edits(&img_data->overlay[F_Prop->active_layer]);
      }
      F_Prop->active_layer = -1;
    }
  }

  // Overlay layers
  for (int i = 0; i < img_data->overlay_count; i++) {
    OverlayLayer *layer = &img_data->overlay[i];
    if (layer->type == LayerType::NONE || layer->srfc == nullptr) {
      continue;
    }

    ImGui::PushID(i);
    // Visibility toggle
    if (ImGui::SmallButton(layer->visible ? "V" : "-")) {
      layer->visible = !layer->visible;
      if (!layer->visible && F_Prop->active_layer == i) {
        // Can't edit invisible layer — switch to map
        F_Prop->active_layer = -1;
      }
      // Upload real or blank data to the texture
      if (layer->visible) {
        Surface *src = ((layer->edit_srfc != nullptr) && (layer->edit_srfc->pxls != nullptr))
                           ? layer->edit_srfc
                           : layer->srfc;
        SURFACE_to_texture(src, layer->texture, src->w, src->h, 1);
      } else {
        int w = layer->srfc->w;
        int h = layer->srfc->h;
        Surface blank_srfc = {};
        blank_srfc.pxls = (uint8_t *)calloc(1, static_cast<size_t>(w) * h);
        blank_srfc.w = static_cast<uint16_t>(w);
        blank_srfc.h = static_cast<uint16_t>(h);
        blank_srfc.pitch = w;
        blank_srfc.channels = 1;
        SURFACE_to_texture(&blank_srfc, layer->texture, w, h, 1);
        free(blank_srfc.pxls);
      }
    }

    ImGui::SameLine();
    bool selected = (F_Prop->active_layer == i);
    if (ImGui::Selectable((layer->name != nullptr) ? layer->name : "Layer", selected)) {
      if (layer->visible) {
        // Commit previous overlay's edits before switching
        if (F_Prop->active_layer >= 0 && F_Prop->active_layer < img_data->overlay_count) {
          commit_layer_edits(&img_data->overlay[F_Prop->active_layer]);
        }
        F_Prop->active_layer = i;
      }
    }
    ImGui::PopID();
  }

  ImGui::Separator();
}

// Commit edit surfaces to edit_data and copy to img_data, then save the
// project. edit_struct is only valid for the file that owns the static edit
// state.
static void commit_and_save_edits(LF *F_Prop, LF *edit_state_owner,
                                  ANM_Dir edit_struct[6]) {
  if (F_Prop == edit_state_owner) {
    commit_map_edits(edit_struct, &F_Prop->edit_data);
    commit_all_overlay_edits(&F_Prop->img_data);
    if ((F_Prop->edit_data.ANM_dir != nullptr) && (F_Prop->img_data.ANM_dir != nullptr)) {
      for (int d = 0; d < 6; d++) {
        int nf = F_Prop->edit_data.ANM_dir[d].num_frames;
        for (int f = 0; f < nf; f++) {
          Surface *src = F_Prop->edit_data.ANM_dir[d].frame_data[f];
          Surface *dst = F_Prop->img_data.ANM_dir[d].frame_data[f];
          if ((src == nullptr) || (dst == nullptr)) {
            continue;
}
          memcpy(dst->pxls, src->pxls, static_cast<size_t>(src->w) * src->h);
        }
      }
    }
  }
  if ((F_Prop->wmap != nullptr) && F_Prop->wmap->save_path[0] != '\0') {
    save_wmap_project(F_Prop->wmap->save_path, F_Prop);
    F_Prop->dirty = false;
  } else if (F_Prop->img_data.type == img_type::FRM && F_Prop->Opened_File[0] != '\0') {
    Save_Info sv_info;
    sv_info.s_type = Save_Type::all_dirs;
    save_FRM_SURFACE(F_Prop->Opened_File, &F_Prop->img_data, &usr_info,
                     &sv_info, true);
    F_Prop->dirty = false;
  }
}

// TODO: store image/editing info in the window itself
void Show_Preview_Window(struct variables *My_Variables, LF *F_Prop,
                         int counter) {
  shader_info *shaders = &My_Variables->shaders;
  image_data *img_data = &F_Prop->img_data;

  // Edit state aliases (file-scope globals, so shutdown can clean up)
  ANM_Dir (&edit_struct)[6] = g_edit_struct;
  StrokeState &stroke_state = g_stroke_state;
  LF *(&edit_state_owner) = g_edit_state_owner;

  std::string a = F_Prop->c_name;
  char b[3];
  sprintf(b, "%02d", counter);
  std::string name = a + "###preview" + b;

  bool was_open = F_Prop->file_open_window;

  if (ImGui::Begin(name.c_str(), (&F_Prop->file_open_window), 0)) {
    // set contextual menu for preview window
    if (ImGui::IsWindowFocused()) {
      My_Variables->window_number_focus = counter;
      My_Variables->edit_image_focused = F_Prop->editing_enabled;
    }
    ImGui::Checkbox("Show Frame Stats", &F_Prop->show_stats);

    ImGui::PushItemWidth(100);
    float *zoom_scale =
        F_Prop->editing_enabled ? &F_Prop->edit_data.scale : &img_data->scale;
    ImGui::DragFloat("##Zoom", zoom_scale, 0.1F, 0.0F, 10.0F, "Zoom: %%%.2fx",
                     0);
    ImGui::PopItemWidth();

    // --- Contextual toolbar for this preview window ---
    {
      Palette *pxlFMT_FO_Pal = My_Variables->FO_Palette;
      image_data *edit_data = &F_Prop->edit_data;

      bool alpha_off = false;
      if (img_data->type == img_type::OTHER) {
        alpha_off = checkbox_handler("Alpha Enabled", &F_Prop->alpha);
        const char *items[] = {"Euclidan Color Matching", "Not Implemented..."};
        ImGui::SameLine();
        ImGui::Combo("##color_match", &My_Variables->color_match_algo, items,
                     IM_ARRAYSIZE(items));
      }

      if (F_Prop->wmap != nullptr) {
        if (!F_Prop->palettized) {
          ImGui::BeginDisabled();
}
        static bool open_wmap_export = false;
        if (ImGui::Button("Export Worldmap")) {
          F_Prop->show_squares = true;
          F_Prop->show_tiles = false;
          // Ensure edit_data is initialized before export
          if (F_Prop->edit_data.ANM_dir == nullptr) {
            prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal,
                               My_Variables->color_match_algo, nullptr,
                               alpha_off);
          }
          commit_map_edits(edit_struct, &F_Prop->edit_data);
          commit_all_overlay_edits(&F_Prop->img_data);
          F_Prop->edit_data.type = img_type::TILE;
          image_data *ed = &F_Prop->edit_data;
          int dir = ed->display_orient_num;
          animate_SURFACE_to_sub_texture(ed, ed->ANM_dir[dir].frame_data[0],
                                         My_Variables->CurrentTime_ms);
          shader_info *shaders = &My_Variables->shaders;
          draw_PAL_to_framebuffer(shaders->FO_pal, shaders->render_PAL_shader,
                                  &shaders->giant_triangle, ed,
                                  F_Prop->img_data.overlay, F_Prop->img_data.overlay_count);
          open_wmap_export = true;
        }
        if (open_wmap_export) {
          open_wmap_export = save_TILE_popup(F_Prop);
        }
        ImGui::SameLine();
        if (ImGui::Button("Export Town-Map Tiles")) {
          F_Prop->show_squares = false;
          F_Prop->show_tiles = true;
          prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal,
                             My_Variables->color_match_algo,
                             &F_Prop->preview_tiles_window, alpha_off);
          F_Prop->show_image_render = false;
        }
        if (!F_Prop->palettized) {
          ImGui::EndDisabled();
}
      }

      if (!F_Prop->editing_enabled) {
        if (img_data->type == img_type::MSK) {
          if (ImGui::Button("Edit MSK file")) {
            prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal,
                               My_Variables->color_match_algo,
                               &F_Prop->editing_enabled, alpha_off);
            // MSK files: set active layer to the MSK overlay
            int msk_idx = find_overlay(F_Prop->img_data.overlay,
                                       F_Prop->img_data.overlay_count, LayerType::MSK);
            F_Prop->active_layer = msk_idx;
          }
        } else if (img_data->type == img_type::OTHER && !F_Prop->palettized) {
          if (ImGui::Button("Palettize Image")) {
            F_Prop->palettized = true;
            for (int i = 0; i < 6; i++) {
              if (edit_data->save_ptr == nullptr) {
                break;
              }
              if (edit_data->save_ptr[i].frame_data != nullptr) {
                free(static_cast<void*>(edit_data->save_ptr[i].frame_data));
                edit_data->save_ptr[i].frame_data = nullptr;
              }
            }

            bool discard = false;
            prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal,
                               My_Variables->color_match_algo, &discard,
                               alpha_off);
          }

          if (ImGui::Button("Convert Image to MSK")) {
            Convert_SURFACE_to_MSK(F_Prop->img_data.ANM_dir[0].frame_data[0],
                                   &F_Prop->img_data, 0);
            prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal,
                               My_Variables->color_match_algo,
                               &F_Prop->editing_enabled, alpha_off);
          }
        }
      }

      if (img_data->type == img_type::OTHER &&
          img_data->ANM_dir[img_data->display_orient_num].num_frames > 1) {
        if (ImGui::Button("Convert Animation to FRM for Editing")) {
          F_Prop->show_image_render = crop_animation_SURFACE(
              img_data, edit_data, My_Variables->FO_Palette, 0,
              &My_Variables->shaders);
        }
      }

      if (img_data->type != img_type::OTHER) {
        if (F_Prop->img_data.ANM_dir == nullptr) {
          ImGui::BeginDisabled();
}
        {
          char png_popup_id[32];
          snprintf(png_popup_id, sizeof(png_popup_id), "Export as PNG##%02d",
                   counter);
          if (ImGui::Button("Export as PNG")) {
            ImGui::OpenPopup(png_popup_id);
          }
          bool open = true;
          if (ImGui::BeginPopupModal(png_popup_id, &open)) {
            open = save_PNG_popup_INTERNAL(img_data, &usr_info);
            ImGui::EndPopup();
          }
        }
        if (F_Prop->img_data.ANM_dir == nullptr) {
          ImGui::EndDisabled();
}
      }

      if (img_data->type != img_type::MSK) {
        if (!F_Prop->editing_enabled) {
          if (ImGui::Button("Enable Editing")) {
            if (img_data->type == img_type::FRM) {
              prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal,
                                 My_Variables->color_match_algo,
                                 &F_Prop->editing_enabled, alpha_off);
            }
            F_Prop->editing_enabled = true;

            // Auto-create MSK overlay for worldmap projects
            image_data *img = &F_Prop->img_data;
            if ((F_Prop->wmap != nullptr) && find_overlay(img->overlay, img->overlay_count, LayerType::MSK) < 0) {
              int idx = add_overlay(img->overlay, &img->overlay_count,
                                    LayerType::MSK, LayerBlend::WHITE_MIX, "Mask",
                                    1.0F, 1.0F, 1.0F, 0.5F);
              if (idx >= 0) {
                img->overlay[idx].srfc = Create_8Bit_Surface(img->width, img->height, nullptr);
                img->overlay[idx].texture = init_texture(img->overlay[idx].srfc,
                                                        img->overlay[idx].srfc->w,
                                                        img->overlay[idx].srfc->h, img_type::MSK);
              }
            }

            // Auto-switch to city layer if a city is selected
            {
              int ci = find_overlay(img->overlay, img->overlay_count, LayerType::CITY);
              if (ci >= 0) {
                city_layer_data* cd = (city_layer_data*)img->overlay[ci].source_data;
                if (cd != nullptr && cd->selected_area >= 0) {
                  F_Prop->active_layer = ci;
                }
              }
            }
          }
        } else {
          if (ImGui::Button("Disable Editing")) {
            commit_map_edits(edit_struct, &F_Prop->edit_data);
            commit_all_overlay_edits(&F_Prop->img_data);

            // Copy map edits to img_data for preview
            if ((F_Prop->edit_data.ANM_dir != nullptr) && (F_Prop->img_data.ANM_dir != nullptr)) {
              for (int d = 0; d < 6; d++) {
                int nf = F_Prop->edit_data.ANM_dir[d].num_frames;
                for (int f = 0; f < nf; f++) {
                  Surface *src = F_Prop->edit_data.ANM_dir[d].frame_data[f];
                  Surface *dst = F_Prop->img_data.ANM_dir[d].frame_data[f];
                  if ((src == nullptr) || (dst == nullptr)) {
                    continue;
}
                  memcpy(dst->pxls, src->pxls, static_cast<size_t>(src->w) * src->h);
                }
              }
            }

            // Sync zoom/pan from edit back to preview
            F_Prop->img_data.scale = F_Prop->edit_data.scale;
            F_Prop->img_data.offset = F_Prop->edit_data.offset;

            F_Prop->editing_enabled = false;
            F_Prop->active_layer = -1;
            My_Variables->edit_image_focused = false;
          }
        }
      }

      // --- Edit toolbar (in top toolbar area) ---
      if (F_Prop->editing_enabled) {
        image_data *ed = &F_Prop->edit_data;

        if (F_Prop->wmap != nullptr) {
          // Layer panel replaces old mask switching buttons
          draw_layer_panel(F_Prop, shaders, &F_Prop->img_data);
        }

        if (ImGui::Button("Reset Image")) {
          stroke_state_cleanup(&stroke_state);
          int num = ed->display_frame_num;
          int dir = ed->display_orient_num;
          bool editing_overlay = (F_Prop->active_layer >= 0 &&
                                  F_Prop->active_layer < F_Prop->img_data.overlay_count);
          Surface *edit_srfc = nullptr;
          if (!editing_overlay) {
            if (edit_struct[dir].frame_data != nullptr) {
              edit_srfc = edit_struct[dir].frame_data[num];
}
          } else {
            edit_srfc = F_Prop->img_data.overlay[F_Prop->active_layer].edit_srfc;
          }
          if (edit_srfc != nullptr) {
            ClearSurface(edit_srfc);
            Surface *src = nullptr;
            GLuint texture = ed->FRM_texture;
            if (editing_overlay) {
              src = F_Prop->img_data.overlay[F_Prop->active_layer].srfc;
              texture = F_Prop->img_data.overlay[F_Prop->active_layer].texture;
            } else {
              src = ed->ANM_dir[dir].frame_data[num];
            }
            if (src != nullptr) {
              memcpy(edit_srfc->pxls, src->pxls, static_cast<size_t>(src->w) * src->h);
              SURFACE_to_texture(edit_srfc, texture, edit_srfc->w, edit_srfc->h, 1);
            }
          }
        }
      }

      if ((F_Prop->wmap == nullptr) && img_data->type == img_type::FRM) {
        static bool open_save = false;
        image_data *ed = &F_Prop->edit_data;
        if (ImGui::Button("Export FRM")) {
          if (ed->ANM_dir == nullptr) {
            prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal,
                               My_Variables->color_match_algo, nullptr,
                               alpha_off);
          }
          commit_map_edits(edit_struct, &F_Prop->edit_data);
          open_save = true;
        }
        if (open_save) {
          int msk_export_idx = find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count, LayerType::MSK);
          if (F_Prop->active_layer >= 0 && F_Prop->active_layer == msk_export_idx) {
            open_save = save_MSK_popup(F_Prop);
          } else if (ed->type == img_type::FRM) {
            open_save = save_FRM_popup(F_Prop);
          } else if (ed->type == img_type::TILE) {
            open_save = save_TILE_popup(F_Prop);
          }
        }
      }

      ImGui::Separator();
    }

    // TODO: show image name for each frame for new animations
    //       this would require attaching the name to each surface
    ImGui::Text("%s", F_Prop->c_name);

    if (F_Prop->editing_enabled) {
      // --- Edit mode ---
      image_data *edit_data = &F_Prop->edit_data;

      if (edit_data->ANM_dir == nullptr) {
        ImGui::Text("No FRM_dir");
      } else if (edit_data->ANM_dir[edit_data->display_orient_num].frame_data ==
                 nullptr) {
        ImGui::Text("No frame_data");
      } else {
        // Initialize edit structures on demand
        if (edit_struct[0].frame_data == nullptr) {
          init_edit_struct_ANM(edit_struct, edit_data,
                               My_Variables->FO_Palette);
          edit_state_owner = F_Prop;
        }
        // Initialize overlay edit surfaces on demand
        for (int oi = 0; oi < F_Prop->img_data.overlay_count; oi++) {
          if ((F_Prop->img_data.overlay[oi].srfc != nullptr) && (F_Prop->img_data.overlay[oi].edit_srfc == nullptr)) {
            init_layer_edit_surface(&F_Prop->img_data.overlay[oi]);
          }
        }

        if (F_Prop->show_stats) {
          show_image_stats_FRM_SURFACE(&F_Prop->edit_data, My_Variables->Font);
        }

        ImVec2 img_pos = display_img_ImGUI(My_Variables, edit_data);

        bool is_interactive = (F_Prop->active_layer >= 0 &&
                               F_Prop->active_layer < F_Prop->img_data.overlay_count &&
                               F_Prop->img_data.overlay[F_Prop->active_layer].interactive);
        if (is_interactive) {
          // Interactive layers don't use Edit_Image, so handle zoom/pan here
          zoom_pan(edit_data, My_Variables->new_mouse_pos, My_Variables->mouse_delta);
        }
        if (is_interactive && F_Prop->img_data.overlay[F_Prop->active_layer].type == LayerType::CITY) {
          Edit_City_Layer(My_Variables, img_pos, &F_Prop->img_data, edit_data, F_Prop->active_layer);
          // Recomposite so overlay texture changes (position, size) show on the map
          shader_info *shaders = &My_Variables->shaders;
          draw_PAL_to_framebuffer(shaders->FO_pal, shaders->render_PAL_shader,
                                  &shaders->giant_triangle, edit_data,
                                  F_Prop->img_data.overlay, F_Prop->img_data.overlay_count);
        } else {
          Edit_Image(My_Variables, img_pos, &F_Prop->edit_data, &F_Prop->img_data,
                     edit_struct, F_Prop->active_layer,
                     My_Variables->Palette_Update, &My_Variables->Color_Pick,
                     &stroke_state);
          draw_brush_cursor(&stroke_state);
          if (!stroke_state.undo_stack.empty()) {
            F_Prop->dirty = true;
          }
        }

        draw_frame_boundary(edit_data, img_pos, F_Prop->active_layer);
        if (My_Variables->pixel_perfect) {
          draw_pixel_grid(edit_data, img_pos, F_Prop->active_layer);
        }

        Gui_Video_Controls(&F_Prop->edit_data, F_Prop->edit_data.type);
      }
    } else {
      // --- Preview mode ---
      // Show overlay visibility toggles in preview mode
      if (img_data->overlay_count > 0 && (F_Prop->wmap != nullptr)) {
        ImGui::Text("Layers");
        for (int oi = 0; oi < img_data->overlay_count; oi++) {
          OverlayLayer *layer = &img_data->overlay[oi];
          if (layer->srfc == nullptr) { continue;
}
          ImGui::SameLine();
          ImGui::PushID(100 + oi);
          if (ImGui::SmallButton(layer->visible ? "V" : "-")) {
            layer->visible = !layer->visible;
            if (layer->visible) {
              SURFACE_to_texture(layer->srfc, layer->texture,
                                 layer->srfc->w, layer->srfc->h, 1);
            } else {
              int w = layer->srfc->w;
              int h = layer->srfc->h;
              Surface blank_srfc = {};
              blank_srfc.pxls = (uint8_t *)calloc(1, static_cast<size_t>(w) * h);
              blank_srfc.w = static_cast<uint16_t>(w);
              blank_srfc.h = static_cast<uint16_t>(h);
              blank_srfc.pitch = w;
              blank_srfc.channels = 1;
              SURFACE_to_texture(&blank_srfc, layer->texture, w, h, 1);
              free(blank_srfc.pxls);
            }
          }
          ImGui::SameLine();
          ImGui::Text("%s", (layer->name != nullptr) ? layer->name : "Overlay");
          ImGui::PopID();
        }
      }

      if (img_data->type == img_type::FRM) {
        // show the original image for previewing
        // TODO: finish setting up usr.info.show_image_stats in settings config
        // in menu
        ImVec2 pre_cursor = ImGui::GetCursorScreenPos();
        preview_FRM_SURFACE(My_Variables, img_data,
                            (F_Prop->show_stats || usr_info.show_image_stats));
        // Allow clicking city markers in preview mode
        int city_idx = find_overlay(img_data->overlay, img_data->overlay_count, LayerType::CITY);
        if (city_idx >= 0 && img_data->overlay[city_idx].visible) {
          ImVec2 img_pos = {pre_cursor.x + img_data->offset.x, pre_cursor.y + img_data->offset.y};
          Edit_City_Layer(My_Variables, img_pos, img_data, img_data, city_idx);
        }

        // gui video controls
        Gui_Video_Controls(img_data, img_data->type);
      } else if (img_data->type == img_type::MSK) {
        Preview_MSK_Image(My_Variables, img_data,
                          (F_Prop->show_stats || usr_info.show_image_stats));
      } else if (img_data->type == img_type::OTHER) {
        ImVec2 pre_cursor = ImGui::GetCursorScreenPos();
        Preview_Image(My_Variables, img_data,
                      (F_Prop->show_stats || usr_info.show_image_stats));
        // Allow clicking city markers in preview mode
        int city_idx = find_overlay(img_data->overlay, img_data->overlay_count, LayerType::CITY);
        if (city_idx >= 0 && img_data->overlay[city_idx].visible) {
          ImVec2 img_pos = {pre_cursor.x + img_data->offset.x, pre_cursor.y + img_data->offset.y};
          Edit_City_Layer(My_Variables, img_pos, img_data, img_data, city_idx);
        }
        // Draw red squares for possible overworld map tiling
        draw_red_squares(img_data, F_Prop->show_squares);

        draw_red_tiles(img_data, F_Prop->show_tiles);

        Gui_Video_Controls(img_data, F_Prop->img_data.type);
      }
    }
  }
  show_popup_warnings();

  // Intercept tab close when there are unsaved edits
  if (was_open && !F_Prop->file_open_window && F_Prop->editing_enabled && F_Prop->dirty) {
    F_Prop->file_open_window = true; // keep window alive
    F_Prop->show_close_confirm = true;
  }

  // Tab close confirmation popup
  char close_popup_id[48];
  snprintf(close_popup_id, sizeof(close_popup_id), "Unsaved Changes##close%02d",
           counter);

  if (F_Prop->show_close_confirm) {
    ImGui::OpenPopup(close_popup_id);
    F_Prop->show_close_confirm = false;
  }
  if (ImGui::BeginPopupModal(close_popup_id, nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("You have unsaved edits.");
    ImGui::Separator();
    if (ImGui::Button("Save & Close")) {
      commit_and_save_edits(F_Prop, edit_state_owner, edit_struct);
      F_Prop->editing_enabled = false;
      F_Prop->active_layer = -1;
      F_Prop->file_open_window = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close without saving")) {
      F_Prop->editing_enabled = false;
      F_Prop->active_layer = -1;
      F_Prop->file_open_window = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  ImGui::End();

  // Commit edits and save before cleanup frees the statics
  if (F_Prop->pending_commit_and_save && !F_Prop->editing_enabled) {
    commit_and_save_edits(F_Prop, edit_state_owner, edit_struct);
    F_Prop->pending_commit_and_save = false;
  }

  // Cleanup when editing is disabled — only for the window that owns the
  // statics
  if (!F_Prop->editing_enabled && F_Prop == edit_state_owner) {
    stroke_state_cleanup(&stroke_state);
    // Cleanup overlay edit surfaces
    for (int oi = 0; oi < F_Prop->img_data.overlay_count; oi++) {
      cleanup_layer_edit_surface(&F_Prop->img_data.overlay[oi]);
    }
    for (int i = 0; i < 6; i++) {
      // Free individual Surface objects before freeing the pointer array
      if (edit_struct[i].frame_data != nullptr) {
        image_data *ed = &F_Prop->edit_data;
        int num_frames = ((ed->ANM_dir != nullptr) && ed->ANM_dir[i].num_frames > 0)
                             ? ed->ANM_dir[i].num_frames
                             : 0;
        for (int f = 0; f < num_frames; f++) {
          if (edit_struct[i].frame_data[f] != nullptr) {
            FreeSurface(edit_struct[i].frame_data[f]);
            edit_struct[i].frame_data[f] = nullptr;
          }
        }
      }
      free(static_cast<void*>(edit_struct[i].frame_data));
      edit_struct[i].frame_data = nullptr;
    }
    edit_state_owner = nullptr;
  }

  // Preview tiles from red boxes
  if (F_Prop->preview_tiles_window) {
    Preview_Tiles_Window(My_Variables, F_Prop, counter);
  }
  // Preview full image
  if (F_Prop->show_image_render) {
    Show_Image_Render(My_Variables, F_Prop, &usr_info, counter);
  }

  if (!F_Prop->file_open_window) {
    // TODO: free img_data?
  }
}

static ImGuiID g_palette_dock_id = 0;

void Show_Palette_Window(variables *My_Variables, bool show_msk_palette) {

  Palette *pal = My_Variables->shaders.FO_pal;

  // Bring palette to foreground when the active layer changes
  {
    static int prev_focus = -1;
    static int prev_layer = -1;
    int cur_focus = My_Variables->window_number_focus;
    int cur_layer = -1;
    if (cur_focus > -1) {
      cur_layer = My_Variables->F_Prop[cur_focus].active_layer;
    }
    if (cur_focus != prev_focus || cur_layer != prev_layer) {
      ImGui::SetNextWindowFocus();
    }
    prev_focus = cur_focus;
    prev_layer = cur_layer;
  }

  bool palette_window = true;
  if (show_msk_palette) {
    ImGui::Begin("MSK colors###palette", &palette_window);

    brush_size_handler(My_Variables);

    ImGui::Text("Erase Mask                    Draw Mask");
    if (ImGui::ColorButton("Erase Mask", ImVec4(0, 0, 0, 1.0F), 0,
                           ImVec2(200.0F, 200.0F))) {
      My_Variables->Color_Pick = (0);
    }
    ImGui::SameLine();
    if (ImGui::ColorButton("Mark Mask", ImVec4(1.0F, 1.0F, 1.0F, 1.0F), 0,
                           ImVec2(200.0F, 200.0F))) {
      My_Variables->Color_Pick = (1);
    }
  } else {
    ImGui::Begin("Default Fallout palette###palette", &palette_window);

    brush_size_handler(My_Variables);

    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {

        int index = (y * 16) + x;

        float r = static_cast<float>(pal->colors[index].r) / 255.0F;
        float g = static_cast<float>(pal->colors[index].g) / 255.0F;
        float b = static_cast<float>(pal->colors[index].b) / 255.0F;

        // give the first button an alpha channel checkerboard
        // TODO: if load_palette_from_path() is changed to use
        //       the first index as alpha = 0 always, then
        //       comment int "float a =" above and delete
        //       the below alpha switch
        float alpha = NAN;
        if (x == 0 && y == 0) {
          alpha = 0.0;
        } else {
          alpha = 1.0;
        }

        char color_info[12];
        snprintf(color_info, 12, "%d##aa%d", index, index);
        if (ImGui::ColorButton(color_info, ImVec4(r, g, b, alpha),
                               ImGuiColorEditFlags_AlphaPreview)) {
          My_Variables->Color_Pick = (uint8_t)(index);
        }

        if (index == My_Variables->Color_Pick) {
          ImVec2 min = ImGui::GetItemRectMin();
          ImVec2 max = ImGui::GetItemRectMax();
          ImDrawList *draw_list = ImGui::GetWindowDrawList();
          draw_list->AddRect(min, max, IM_COL32(0, 0, 0, 255), 0.0F, 0, 2.0F);
          draw_list->AddRect(min, max, IM_COL32(255, 255, 255, 255), 0.0F, 0,
                             1.0F);
        }

        if (x < 15) {
          ImGui::SameLine();
        }
      }
    }
  }

  g_palette_dock_id = ImGui::GetWindowDockID();
  ImGui::End();
}

void Show_City_Info_Window(variables *My_Variables) {
  static int prev_selected = -1;
  OverlayLayer* city_layer = nullptr;
  city_layer_data* city_data = nullptr;
  LF* focused = nullptr;

  if (My_Variables->window_number_focus > -1) {
    focused = &My_Variables->F_Prop[My_Variables->window_number_focus];
    int city_idx = find_overlay(focused->img_data.overlay,
                                focused->img_data.overlay_count, LayerType::CITY);
    if (city_idx >= 0) {
      city_layer = &focused->img_data.overlay[city_idx];
      city_data = (city_layer_data*)city_layer->source_data;
    }
  }

  bool has_selected_city = (city_data != nullptr && city_data->selected_area >= 0 &&
                            city_data->selected_area < city_data->area_count);

  if (!has_selected_city) {
    prev_selected = -1;
    return;
  }

  // Dock into the same node as the palette when the window first appears
  if (g_palette_dock_id != 0) {
    ImGui::SetNextWindowDockID(g_palette_dock_id, ImGuiCond_Appearing);
  }

  bool editing = focused->editing_enabled && focused->active_layer >= 0 &&
                 focused->img_data.overlay[focused->active_layer].type == LayerType::CITY;

  // Auto-focus the window when a new city is selected or editing starts
  static bool prev_editing = false;
  if (city_data->selected_area != prev_selected || (editing && !prev_editing)) {
    ImGui::SetNextWindowFocus();
  }
  prev_selected = city_data->selected_area;
  prev_editing = editing;

  bool open = true;
  ImGui::Begin("City Info", &open);
  if (draw_city_info_panel(city_layer, editing)) {
      focused->dirty = true;
  }
  ImGui::End();
}

void Preview_Tiles_Window(variables *My_Variables, LF *F_Prop, int counter) {
  std::string image_name = F_Prop->c_name;
  image_data *edit_data = &F_Prop->edit_data;
  char window_id[3];
  sprintf(window_id, "%02d", counter);
  std::string name = image_name + " Preview...###render" + window_id;

  if (edit_data->type != img_type::TILE) {
    edit_data->type = img_type::TILE;
  }

  // shortcuts
  if (ImGui::Begin(name.c_str(), &F_Prop->preview_tiles_window, 0)) {

    ImGui::PushItemWidth(100);
    ImGui::DragFloat("##Zoom", &edit_data->scale, 0.1F, 0.0F, 10.0F,
                     "Zoom: %%%.2fx", 0);
    ImGui::PopItemWidth();

    if (ImGui::IsWindowFocused()) {
      My_Variables->window_number_focus = counter;
      My_Variables->tile_window_focused = true;
      My_Variables->render_wind_focused = false;
    }

    prev_TMAP_tiles_SURFACE(&usr_info, My_Variables, edit_data);
  }
  ImGui::End();
}

void Show_Image_Render(variables *My_Variables, LF *F_Prop,
                       struct user_info *usr_info, int counter) {
  image_data *edit_data = &F_Prop->edit_data;
  char b[3];
  sprintf(b, "%02d", counter);
  std::string a = F_Prop->c_name;
  std::string name = a + "Render Window...###render" + b;

  if (ImGui::Begin(name.c_str(), &F_Prop->show_image_render, 0)) {
    if (ImGui::IsWindowFocused()) {
      My_Variables->window_number_focus = counter;
      My_Variables->render_wind_focused = true;
      My_Variables->tile_window_focused = false;
    }
    ImGui::PushItemWidth(100);
    ImGui::DragFloat("##Zoom", &edit_data->scale, 0.1F, 0.0F, 10.0F,
                     "Zoom: %%%.2fx", 0);
    ImGui::PopItemWidth();
    ImGui::Checkbox("Show Frame Stats", &F_Prop->show_stats);

    preview_FRM_SURFACE(My_Variables, edit_data,
                        (F_Prop->show_stats || usr_info->show_image_stats));

    Gui_Video_Controls(edit_data, edit_data->type);
  }
  ImGui::End();
}

// TODO: Need to test wide character support
void Open_Files(struct user_info *usr_info, int *counter, Palette *pxlFMT,
                struct variables *My_Variables) {
  LF *F_Prop = &My_Variables->F_Prop[*counter];
  // Assigns image to Load_Files.image and loads palette for the image
  // TODO: image needs to be less than 1 million pixels (1000x1000)
  // to be viewable in Titanium FRM viewer, what's the limit in the game?
  // (limit is greater than 1600x1200 for Hi-Res mod - tested on MR f2_res.dat)
  F_Prop->file_open_window = ImDialog_load_files(
      F_Prop, &F_Prop->img_data, usr_info, &My_Variables->shaders,
      My_Variables->F_Prop, *counter);

  if (My_Variables->F_Prop[*counter].c_name != nullptr) {
    (*counter)++;
  }
}

static void ShowShortcutsWindow(bool *p_open) {
  ImGui::SetNextWindowSize(ImVec2(480, 400), ImGuiCond_FirstUseEver);
  ImGui::Begin("Shortcuts", p_open);
  if (ImGui::BeginTable("shortcuts_table", 3,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Category");
    ImGui::TableSetupColumn("Shortcut");
    ImGui::TableSetupColumn("Action");
    ImGui::TableHeadersRow();

    struct {
      const char *category;
      const char *shortcut;
      const char *action;
    } entries[] = {
        {"File", "Ctrl+O", "Open file"},
        {"Edit", "Ctrl+Z", "Undo"},
        {"Edit", "Ctrl+Y", "Redo"},
        {"Animation", "Space", "Play/Pause animation"},
        {"Animation", "Left Arrow", "Previous frame"},
        {"Animation", "Right Arrow", "Next frame"},
        {"Animation", "Up Arrow", "Next orientation"},
        {"Animation", "Down Arrow", "Previous orientation"},
        {"View", "Ctrl+Mouse Wheel", "Zoom in/out"},
        {"View", "Right Mouse Drag", "Pan image"},
        {"Editing", "Escape", "Cancel stroke"},
        {"Editing", "Right Click (in stroke)", "Cancel stroke"},
        {"General", "Escape", "Close application"},
    };

    for (auto &e : entries) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(e.category);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(e.shortcut);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(e.action);
    }

    ImGui::EndTable();
  }
  ImGui::End();
}

static void NewWmapProject_Dialogs(int *counter,
                                   struct variables *My_Variables) {
  // Handle file picker result for New Worldmap Project
  if (ifd::FileDialog::Instance().IsDone("NewWmapImageDialog")) {
    if (ifd::FileDialog::Instance().HasResult()) {
      std::string path = ifd::FileDialog::Instance().GetResult().u8string();

      // Load as RGBA
      Surface *rgba = Load_File_to_RGBA(path.c_str());
      if (rgba == nullptr) {
        set_popup_warning("[ERROR] New Worldmap Project\n\n"
                          "Unable to load the selected image.");
      } else if (rgba->w % WMAP_TILE_W != 0 || rgba->h % WMAP_TILE_H != 0) {
        set_popup_warning("[ERROR] New Worldmap Project\n\n"
                          "Image dimensions must be multiples\n"
                          "of 350x300 pixels.");
        FreeSurface(rgba);
      } else {
        g_new_wmap_source = rgba;
        g_new_wmap_tiles_x = rgba->w / WMAP_TILE_W;
        g_new_wmap_tiles_y = rgba->h / WMAP_TILE_H;
        strncpy(g_new_wmap_base_name, "WRLDMP", 7);
        g_new_wmap_pending = true;
        ImGui::OpenPopup("New Worldmap Project");
      }
    }
    ifd::FileDialog::Instance().Close();
  }

  // Popup modal for New Worldmap Project
  if (g_new_wmap_pending) {
    ImGui::OpenPopup("New Worldmap Project");
  }
  if (ImGui::BeginPopupModal("New Worldmap Project", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Image size: %dx%d pixels",
                (g_new_wmap_source != nullptr) ? g_new_wmap_source->w : 0,
                (g_new_wmap_source != nullptr) ? g_new_wmap_source->h : 0);
    ImGui::Text("Grid: %d x %d tiles", g_new_wmap_tiles_x, g_new_wmap_tiles_y);
    ImGui::Separator();

    ImGui::Text("Project name:");
    ImGui::InputText("##wmap_base_name", g_new_wmap_base_name,
                     sizeof(g_new_wmap_base_name));

    ImGui::Separator();
    if (ImGui::Button("OK", ImVec2(120, 0))) {
      if (g_new_wmap_source != nullptr) {
        // Palettize the RGBA source to 8-bit indexed
        Surface *indexed =
            PAL_Color_Convert(g_new_wmap_source, My_Variables->FO_Palette, 0);
        FreeSurface(g_new_wmap_source);
        g_new_wmap_source = nullptr;

        if (indexed != nullptr) {
          LF *F_Prop = &My_Variables->F_Prop[*counter];
          bool ok = new_wmap_project(
              F_Prop, &F_Prop->img_data, &My_Variables->shaders, indexed,
              g_new_wmap_base_name, g_new_wmap_tiles_x, g_new_wmap_tiles_y);
          FreeSurface(indexed);
          if (ok) {
            // Set display name
            static char wmap_name[] = "Worldmap Project";
            F_Prop->c_name = wmap_name;
            (*counter)++;
          }
        }
      }
      g_new_wmap_pending = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      if (g_new_wmap_source != nullptr) {
        FreeSurface(g_new_wmap_source);
        g_new_wmap_source = nullptr;
      }
      g_new_wmap_pending = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  // Handle Open Worldmap Project dialog result
  if (ifd::FileDialog::Instance().IsDone("OpenWmapDialog")) {
    if (ifd::FileDialog::Instance().HasResult()) {
      std::string path = ifd::FileDialog::Instance().GetResult().u8string();

      // Update default load path
      strncpy(usr_info.default_load_path, path.c_str(), MAX_PATH);
      char *ptr = strrchr(usr_info.default_load_path, PLATFORM_SLASH);
      if (ptr != nullptr) {
        *ptr = '\0';
}

      // Check if already open
      int existing =
          find_open_file(My_Variables->F_Prop, *counter, path.c_str());
      if (existing >= 0) {
        focus_file_window(existing);
        add_recent_file(&usr_info, path.c_str());
      } else {
        LF *F_Prop = &My_Variables->F_Prop[*counter];
        if (File_Type_Check(F_Prop, &My_Variables->shaders, &F_Prop->img_data,
                            path.c_str())) {
          add_recent_file(&usr_info, path.c_str());
          (*counter)++;
        }
      }
    }
    ifd::FileDialog::Instance().Close();
  }

  // Handle Save As dialog result
  if (ifd::FileDialog::Instance().IsDone("WmapSaveDialog")) {
    if (ifd::FileDialog::Instance().HasResult()) {
      std::string save_path =
          ifd::FileDialog::Instance().GetResult().u8string();

      // Ensure .wmap extension
      if (save_path.size() < 5 ||
          save_path.substr(save_path.size() - 5) != ".wmap") {
        save_path += ".wmap";
      }

      int focus = My_Variables->window_number_focus;
      if (focus >= 0 && (My_Variables->F_Prop[focus].wmap != nullptr)) {
        if (save_wmap_project(save_path.c_str(),
                              &My_Variables->F_Prop[focus])) {
          add_recent_file(&usr_info, save_path.c_str());
          // Register the save path so find_open_file() can detect
          // this project is already open (prevents duplicate tabs
          // when loading the same .wmap from recent files)
          LF *fp = &My_Variables->F_Prop[focus];
          strncpy(fp->Opened_File, save_path.c_str(), MAX_PATH - 1);
          fp->Opened_File[MAX_PATH - 1] = '\0';
          // Update tab name to the filename
          char *slash = strrchr(fp->Opened_File, PLATFORM_SLASH);
          fp->c_name = (slash != nullptr) ? slash + 1 : fp->Opened_File;
        }

        // Update default save path
        strncpy(usr_info.default_save_path, save_path.c_str(), MAX_PATH);
        char *ptr = strrchr(usr_info.default_save_path, '/');
#ifdef QFO2_WINDOWS
        char *bptr = strrchr(usr_info.default_save_path, '\\');
        if (bptr > ptr)
          ptr = bptr;
#endif
        if (ptr != nullptr) {
          *ptr = '\0';
}
      }
    }
    ifd::FileDialog::Instance().Close();
  }

  // Handle Import Worldmap from FO2 folder picker result
  if (ifd::FileDialog::Instance().IsDone("ImportWmapFolderDialog")) {
    if (ifd::FileDialog::Instance().HasResult()) {
      std::string path = ifd::FileDialog::Instance().GetResult().u8string();
      printf("ImportWmapFolderDialog: selected path = '%s'\n", path.c_str());

      // Check if this is the game root (containing data/worldmap.txt)
      // or the data/ folder itself (containing worldmap.txt directly).
      // Uses case-insensitive path resolution for Linux compatibility
      // with extracted Fallout 2 archives (which use ALL CAPS filenames).
      char wmap_check[MAX_PATH];
      bool found_root = false;

      // Check 1: user selected game root -> data/worldmap.txt exists under it
      if (resolve_path_icase(path.c_str(), "data/worldmap.txt", wmap_check,
                             MAX_PATH)) {
        printf("  -> game root detected\n");
        strncpy(g_import_wmap_data_path, path.c_str(), MAX_PATH - 1);
        g_import_wmap_data_path[MAX_PATH - 1] = '\0';
        strncpy(g_import_wmap_base_name, "WRLDMP", 7);
        g_import_wmap_pending = true;
        found_root = true;
      }

      // Check 2: user selected the data/ folder itself -> worldmap.txt is
      // directly inside
      if (!found_root && resolve_path_icase(path.c_str(), "worldmap.txt",
                                            wmap_check, MAX_PATH)) {
        // Strip trailing slashes, then strip the last path component
        // (the data/ dir) to get game root
        char parent[MAX_PATH];
        strncpy(parent, path.c_str(), MAX_PATH - 1);
        parent[MAX_PATH - 1] = '\0';
        int plen = static_cast<int>(strlen(parent));
        while (plen > 1 &&
               (parent[plen - 1] == '/' || parent[plen - 1] == '\\')) {
          parent[--plen] = '\0';
}
        char *last_slash = strrchr(parent, '/');
#ifdef QFO2_WINDOWS
        char *last_bslash = strrchr(parent, '\\');
        if (last_bslash > last_slash)
          last_slash = last_bslash;
#endif
        if (last_slash != nullptr) {
          *last_slash = '\0';
}

        printf("  -> data folder detected, using parent: %s\n", parent);
        strncpy(g_import_wmap_data_path, parent, MAX_PATH - 1);
        g_import_wmap_data_path[MAX_PATH - 1] = '\0';
        strncpy(g_import_wmap_base_name, "WRLDMP", 7);
        g_import_wmap_pending = true;
        found_root = true;
      }

      if (!found_root) {
        snprintf(g_import_error, sizeof(g_import_error),
                 "worldmap.txt not found. Please select the Fallout 2 data/ "
                 "folder.");
        g_import_error_pending = true;
      }
    } else {
      printf("ImportWmapFolderDialog: IsDone but no result (cancelled or "
             "empty)\n");
    }
    ifd::FileDialog::Instance().Close();
  }

  // Import Worldmap from FO2 confirmation popup
  if (g_import_wmap_pending) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    ImGui::OpenPopup("Import Worldmap from FO2");
  }
  if (ImGui::BeginPopupModal("Import Worldmap from FO2", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Data folder: %s", g_import_wmap_data_path);
    ImGui::Separator();

    ImGui::Text("Project name:");
    ImGui::InputText("##import_wmap_base_name", g_import_wmap_base_name,
                     sizeof(g_import_wmap_base_name));

    ImGui::Separator();
    if (!g_import_wmap_done) {
      if (ImGui::Button("Import", ImVec2(120, 0))) {
        printf("Import button clicked: path='%s' name='%s'\n",
               g_import_wmap_data_path, g_import_wmap_base_name);
        g_import_error[0] = '\0';
        int msk_skipped = 0;
        LF *F_Prop = &My_Variables->F_Prop[*counter];
        bool ok = import_wmap_from_fo2(
            g_import_wmap_data_path, g_import_wmap_base_name, F_Prop,
            &F_Prop->img_data, &My_Variables->shaders, &msk_skipped);
        printf("import_wmap_from_fo2 returned: %s\n", ok ? "true" : "false");
        if (ok) {
          static char import_wmap_name[] = "Worldmap Project";
          F_Prop->c_name = import_wmap_name;
          (*counter)++;
          if (msk_skipped > 0) {
            snprintf(g_import_error, sizeof(g_import_error),
                     "Import succeeded, but %d MSK mask file(s) could not be "
                     "found or loaded. "
                     "The worldmap mask layer may be incomplete.",
                     msk_skipped);
            g_import_wmap_done = true;
          } else {
            g_import_wmap_pending = false;
            ImGui::CloseCurrentPopup();
          }
        } else {
          snprintf(g_import_error, sizeof(g_import_error), "%s",
                   get_popup_warning_text());
        }
      }
      ImGui::SameLine();
    }
    if (ImGui::Button(g_import_wmap_done ? "OK" : "Cancel", ImVec2(120, 0))) {
      g_import_wmap_pending = false;
      g_import_wmap_done = false;
      g_import_error[0] = '\0';
      ImGui::CloseCurrentPopup();
    }

    if (g_import_error[0] != '\0') {
      ImGui::Separator();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.3F, 0.3F, 1.0F));
      ImGui::TextWrapped("%s", g_import_error);
      ImGui::PopStyleColor();
    }

    ImGui::EndPopup();
  }

  // Folder validation error popup
  if (g_import_error_pending) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    ImGui::OpenPopup("Import Worldmap Error");
    g_import_error_pending = false;
  }
  ImGui::SetNextWindowSizeConstraints(ImVec2(400, 0), ImVec2(FLT_MAX, FLT_MAX));
  if (ImGui::BeginPopupModal("Import Worldmap Error", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("%s", g_import_error);
    ImGui::Separator();
    if (ImGui::Button("OK", ImVec2(120, 0))) {
      g_import_error[0] = '\0';
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

static void ShowMainMenuBar(int *counter, struct variables *My_Variables) {
  static bool show_shortcuts = false;
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Worldmap Project")) {
        init_IFD();
        ifd::FileDialog::Instance().Open(
            "NewWmapImageDialog", "Select Source Image",
            "Image "
            "(*.png;*.bmp;*.jpg){.png,.PNG,.bmp,.BMP,.jpg,.JPG,.jpeg,.JPEG}",
            false, usr_info.default_load_path);
      }
      if (ImGui::MenuItem("Open Worldmap Project", "Ctrl+O")) {
        init_IFD();
        ifd::FileDialog::Instance().Open(
            "OpenWmapDialog", "Open Worldmap Project",
            "Worldmap Project (*.wmap){.wmap,.WMAP}", false,
            usr_info.default_load_path);
      }
      if (ImGui::MenuItem("Import Worldmap from FO2")) {
        init_IFD();
        ifd::FileDialog::Instance().Open(
            "ImportWmapFolderDialog", "Select Fallout 2 data/ folder", "",
            false,
            (usr_info.default_game_path[0] != 0) ? usr_info.default_game_path
                                          : usr_info.default_load_path);
      }
      // File->Save: enabled only when focused window is a worldmap project
      {
        int focus = My_Variables->window_number_focus;
        bool can_save =
            (focus >= 0 && My_Variables->F_Prop[focus].wmap != nullptr);
        if (ImGui::MenuItem("Save Project", "Ctrl+S", false, can_save) && can_save) {
          LF *fp = &My_Variables->F_Prop[focus];
          if (fp->wmap->save_path[0] != '\0') {
            save_wmap_project(fp->wmap->save_path, fp);
          } else {
            init_IFD();
            ifd::FileDialog::Instance().Save(
                "WmapSaveDialog", "Save Worldmap Project",
                "Worldmap Project (*.wmap){.wmap,.WMAP}",
                usr_info.default_save_path);
          }
        }
      }
      if (ImGui::MenuItem("Set Fallout2.exe Path")) {
        Set_Default_Game_Path(&usr_info, My_Variables->exe_directory);
      }
      // TODO: implement "Open Recent" menu
      // if (ImGui::BeginMenu("Open Recent")) {}
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "CTRL+Z")) {
        My_Variables->undo_requested = true;
      }
      if (ImGui::MenuItem("Redo", "CTRL+Y")) {
        My_Variables->redo_requested = true;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Cut", "CTRL+X", false, false)) {
      }
      if (ImGui::MenuItem("Copy", "CTRL+C", false, false)) {
      }
      if (ImGui::MenuItem("Paste", "CTRL+V", false, false)) {
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Config")) {
      if (ImGui::MenuItem("Reset ImGui.ini")) {
        g_reset_imgui_ini = true;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(g_reset_imgui_ini
                              ? "Restart the application to apply the reset."
                              : "Reset window layout to defaults.\n"
                                "Requires an application restart.");
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("Shortcuts", "", show_shortcuts)) {
        show_shortcuts = !show_shortcuts;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
  if (show_shortcuts) {
    ShowShortcutsWindow(&show_shortcuts);
  }
  set_game_path_POPUP(&usr_info);
  game_path_set_POPUP(&usr_info);
  game_path_NOT_set_POPUP();
  NewWmapProject_Dialogs(counter, My_Variables);

  // Ctrl+S shortcut for saving worldmap projects
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S,
                      ImGuiInputFlags_RouteGlobal)) {
    int focus = My_Variables->window_number_focus;
    if (focus >= 0 && (My_Variables->F_Prop[focus].wmap != nullptr)) {
      LF *fp = &My_Variables->F_Prop[focus];
      if (fp->wmap->save_path[0] != '\0') {
        save_wmap_project(fp->wmap->save_path, fp);
      } else {
        init_IFD();
        ifd::FileDialog::Instance().Save(
            "WmapSaveDialog", "Save Worldmap Project",
            "Worldmap Project (*.wmap){.wmap,.WMAP}",
            usr_info.default_save_path);
      }
    }
  }
}

bool save_FRM_popup(LF *F_Prop) {
  image_data *img_data = &F_Prop->edit_data;

  Save_Info sv_info;
  bool open_window = true;
  // TODO: replace ImGui::Begin() with BeginPopupModal()?
  ImGui::Begin("Export FRM", &open_window);
  static int e;
  ImGui::RadioButton("Selected Frame", &e, 0);
  ImGui::RadioButton("Selected Direction", &e, 1);
  ImGui::RadioButton("All Directions", &e, 2);
  sv_info.s_type = (Save_Type)e;

  char dup_name[MAX_PATH] = {};

  if (open_window) {
    open_window = ImDialog_save_FRM_SURFACE(img_data, &usr_info, &sv_info);
  }
  ImGui::End();

  return open_window;
}

bool save_MSK_popup(LF *F_Prop) {
  image_data *img_data = &F_Prop->img_data;
  Save_Info *sv_info = nullptr;

  bool open_window = true;
  // TODO: replace ImGui::Begin() with BeginPopupModal()?
  ImGui::Begin("Export MSK", &open_window);
  if (open_window) {
    open_window = ImDialog_save_TILE_SURFACE(img_data, &usr_info, sv_info);
  }
  ImGui::End();

  return open_window;
}

bool save_TILE_popup(LF *F_Prop) {
  image_data *img_data = &F_Prop->edit_data;
  Save_Info *sv_info = {};

  bool open_window = true;
  // TODO: replace ImGui::Begin() with BeginPopupModal()?
  ImGui::Begin("Export Worldmap", &open_window);
  if (open_window) {
    int msk_i = find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count, LayerType::MSK);
    Surface *msk = (msk_i >= 0) ? F_Prop->img_data.overlay[msk_i].srfc : nullptr;
    const char *preset = (F_Prop->wmap != nullptr) ? "WRLDMP" : nullptr;
    int city_i = find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count, LayerType::CITY);
    city_layer_data *city =
        (city_i >= 0) ? (city_layer_data *)F_Prop->img_data.overlay[city_i].source_data : nullptr;
    open_window =
        ImDialog_save_TILE_SURFACE(img_data, &usr_info, sv_info, msk, preset, city);
  }
  ImGui::End();

  return open_window;
}

void main_window_bttns(variables *My_Variables, int *counter) {
  LF *F_Prop = &My_Variables->F_Prop[*counter];
  image_data *img_data = &F_Prop->img_data;

  bool success =
      ImDialog_load_files(F_Prop, img_data, &usr_info, &My_Variables->shaders,
                          My_Variables->F_Prop, *counter);
  if (success) {
    (*counter)++;
  }
  ImGui::Separator();

  char recent_file_warning[MAX_PATH + 64] = {};
  if (usr_info.recent_files_count > 0) {
    ImGui::Text("Recent Files:");
    for (int i = 0; i < usr_info.recent_files_count; i++) {
      ImGui::PushID(i);
      const char *full_path = usr_info.recent_files[i];
      const char *filename = strrchr(full_path, PLATFORM_SLASH);
      filename = (filename != nullptr) ? filename + 1 : full_path;

      if (ImGui::Selectable(filename)) {
        int existing =
            find_open_file(My_Variables->F_Prop, *counter, full_path);
        if (existing >= 0) {
          focus_file_window(existing);
          add_recent_file(&usr_info, full_path);
        } else if (!io_file_exists(full_path)) {
          snprintf(recent_file_warning, sizeof(recent_file_warning),
                   "File not found:\n%s", full_path);
          remove_recent_file(&usr_info, i);
          ImGui::PopID();
          break;
        } else {
          F_Prop = &My_Variables->F_Prop[*counter];
          F_Prop->file_open_window = File_Type_Check(
              F_Prop, &My_Variables->shaders, &F_Prop->img_data, full_path);
          if (F_Prop->file_open_window) {
            add_recent_file(&usr_info, full_path);
            (*counter)++;
          }
        }
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", full_path);
      }
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Selectable("Remove")) {
          remove_recent_file(&usr_info, i);
          ImGui::EndPopup();
          ImGui::PopID();
          break;
        }
        ImGui::EndPopup();
      }
      ImGui::PopID();
    }
  }
  if (recent_file_warning[0] != 0) {
    set_popup_warning(recent_file_warning);
  }
}

#ifdef QFO2_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine,
                   INT nCmdShow) {
  // int size = GetCurrentDirectory(0, NULL);
  // char* buffer = (char*)malloc(size*(sizeof(char)));
  // GetCurrentDirectory(size, buffer);

  return main(0, NULL);
}
#endif
