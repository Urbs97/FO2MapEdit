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
#include <cstdio>

#include <formats/MiniSDL.h>

// TODO: fix this so it compiles for windows
#ifdef QFO2_WINDOWS
#include <Windows.h>
#endif

// My header files
#include "Image2Texture.h"
#include "Load_Files.h"
#include "Load_Settings.h"
#include "platform/platform_io.h"

#include "dat2/dat2_tree_view.h"
#include "rendering/display_FRM_OpenGL.h"
#include "formats/FRM_Convert.h"
#include "formats/Palette_Cycle.h"
#include "Stroke_State.h"

#include "App_State.h"
#include "ui/Show_DAT_Window.h"
#include "ui/Show_Main_Menu.h"
#include "ui/Show_Palette_Window.h"
#include "ui/Show_Preview_Window.h"
#include "ui/ImGui_Warning.h"
#include "file_types/File_Type_Registry.h"
#include "platform/timer_functions.h"

// (edit_struct and stroke_state are now per-window fields on LF)



static void glfw_error_callback(int error, const char *description) {
  // Suppress GLFW_FEATURE_UNAVAILABLE (65548) for Wayland window position
  // warnings
  if (error == 65548) {
    return;
}
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}


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

  // Merge Nerd Font Symbols (icons) into the default font
  {
    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.PixelSnapH = true;
    cfg.GlyphOffset.y = -2.0F; // nudge icons up to align with text baseline
    static const ImWchar icon_ranges[] = { 0xe000, 0xf2ff, 0 };
    char nf_path[MAX_PATH];
    snprintf(nf_path, sizeof(nf_path), "%s%s", My_Variables.exe_directory,
             "resources//fonts//SymbolsNerdFontMono-Regular.ttf");
    io.Fonts->AddFontFromFileTTF(nf_path, static_cast<float>(My_Variables.global_font_size) * 0.6F,
                                 &cfg, icon_ranges);
  }

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

    // contextual palette window — switches based on active layer type
    {
      LayerType palette_layer = LayerType::NONE;
      if (My_Variables.window_number_focus > -1) {
        LF* focused = &My_Variables.F_Prop[My_Variables.window_number_focus];
        int al = focused->active_layer;
        if (al >= 0 && al < focused->img_data.overlay_count) {
          palette_layer = focused->img_data.overlay[al].type;
        }
      }
      Show_Palette_Window(&My_Variables, palette_layer);
      Show_City_Info_Window(&My_Variables);
    }

    // update palette at regular intervals
    My_Variables.Palette_Update = update_PAL_array(My_Variables.shaders.FO_pal,
                                                   static_cast<double>(My_Variables.CurrentTime_ms));

    for (int i = 0; i < counter; i++) {
      if (My_Variables.F_Prop[i].file_open_window) {
        if (My_Variables.F_Prop[i].dat != nullptr) {
          Show_DAT_Window(&My_Variables, &My_Variables.F_Prop[i], i, &counter);
        } else {
          Show_Preview_Window(&My_Variables, &My_Variables.F_Prop[i], i);
        }
      }
    }

    // Reset focus if the focused window is no longer open
    if (My_Variables.window_number_focus >= 0 &&
        !My_Variables.F_Prop[My_Variables.window_number_focus].file_open_window) {
      My_Variables.window_number_focus = -1;
      My_Variables.edit_image_focused = false;
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

  for (auto & i : My_Variables.F_Prop) {
    stroke_state_cleanup(&i.stroke_state);
    // Free per-window edit_struct surfaces
    for (int d = 0; d < 6; d++) {
      if (i.edit_struct[d].frame_data != nullptr) {
        int nf = (i.edit_data.ANM_dir != nullptr && i.edit_data.ANM_dir[d].num_frames > 0)
                     ? i.edit_data.ANM_dir[d].num_frames : 0;
        for (int f = 0; f < nf; f++) {
          if (i.edit_struct[d].frame_data[f] != nullptr) {
            FreeSurface(i.edit_struct[d].frame_data[f]);
          }
        }
        free(static_cast<void*>(i.edit_struct[d].frame_data));
        i.edit_struct[d].frame_data = nullptr;
      }
    }
    Clear_img_data(&i.img_data);
    Clear_img_data(&i.edit_data);
    free(i.wmap);
    i.wmap = nullptr;
    free(i.ssl_text);
    i.ssl_text = nullptr;
    delete i.dat;
    i.dat = nullptr;
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


#ifdef QFO2_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine,
                   INT nCmdShow) {
  // int size = GetCurrentDirectory(0, NULL);
  // char* buffer = (char*)malloc(size*(sizeof(char)));
  // GetCurrentDirectory(size, buffer);

  return main(0, NULL);
}
#endif
