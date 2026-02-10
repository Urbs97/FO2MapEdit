#include "Show_Main_Menu.h"

#include "../App_State.h"
#include "../FRM_Convert.h"
#include "../Image2Texture.h"
#include "../Load_Files.h"
#include "../Load_Settings.h"
#include "../Save_Files.h"
#include "../Worldmap_Project.h"
#include "../dat2/dat2_writer.h"
#include "../file_types/File_Type_Registry.h"
#include "../platform_io.h"
#include "ImGui_Warning.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <ImFileDialog.h>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>

// File->New Worldmap Project state
static bool g_new_wmap_pending = false;
static Surface* g_new_wmap_source = nullptr;
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

// File->Create DAT2 Archive state
static bool g_create_dat2_folder_selected = false;
static char g_create_dat2_folder_path[MAX_PATH] = "";
static bool g_create_dat2_compress = true;

// TODO: Need to test wide character support
void Open_Files(struct user_info* usr_info, int* counter, Palette* pxlFMT,
                struct variables* My_Variables) {
    LF* F_Prop = &My_Variables->F_Prop[*counter];
    // Assigns image to Load_Files.image and loads palette for the image
    // TODO: image needs to be less than 1 million pixels (1000x1000)
    // to be viewable in Titanium FRM viewer, what's the limit in the game?
    // (limit is greater than 1600x1200 for Hi-Res mod - tested on MR f2_res.dat)
    F_Prop->file_open_window =
        ImDialog_load_files(F_Prop, &F_Prop->img_data, usr_info, &My_Variables->shaders,
                            My_Variables->F_Prop, *counter);

    if (My_Variables->F_Prop[*counter].c_name != nullptr) {
        (*counter)++;
    }
}

static void ShowShortcutsWindow(bool* p_open) {
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
            const char* category;
            const char* shortcut;
            const char* action;
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
            {"Editing", "Escape", "Exit edit mode / Cancel stroke"},
            {"Editing", "Right Click (in stroke)", "Cancel stroke"},
        };

        for (auto& e : entries) {
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

static void NewWmapProject_Dialogs(int* counter, struct variables* My_Variables) {
    // Handle file picker result for New Worldmap Project
    if (ifd::FileDialog::Instance().IsDone("NewWmapImageDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            std::string path = ifd::FileDialog::Instance().GetResult().u8string();

            // Load as RGBA
            Surface* rgba = Load_File_to_RGBA(path.c_str());
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
        ImGui::InputText("##wmap_base_name", g_new_wmap_base_name, sizeof(g_new_wmap_base_name));

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            if (g_new_wmap_source != nullptr) {
                // Palettize the RGBA source to 8-bit indexed
                Surface* indexed =
                    PAL_Color_Convert(g_new_wmap_source, My_Variables->FO_Palette, 0);
                FreeSurface(g_new_wmap_source);
                g_new_wmap_source = nullptr;

                if (indexed != nullptr) {
                    LF* F_Prop = &My_Variables->F_Prop[*counter];
                    bool ok = new_wmap_project(F_Prop, &F_Prop->img_data, &My_Variables->shaders,
                                               indexed, g_new_wmap_base_name, g_new_wmap_tiles_x,
                                               g_new_wmap_tiles_y);
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
            char* ptr = strrchr(usr_info.default_load_path, PLATFORM_SLASH);
            if (ptr != nullptr) {
                *ptr = '\0';
            }

            // Check if already open
            int existing = find_open_file(My_Variables->F_Prop, *counter, path.c_str());
            if (existing >= 0) {
                focus_file_window(existing);
                add_recent_file(&usr_info, path.c_str());
            } else {
                LF* F_Prop = &My_Variables->F_Prop[*counter];
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
            std::string save_path = ifd::FileDialog::Instance().GetResult().u8string();

            // Ensure .wmap extension
            if (save_path.size() < 5 || save_path.substr(save_path.size() - 5) != ".wmap") {
                save_path += ".wmap";
            }

            int focus = My_Variables->window_number_focus;
            if (focus >= 0 && (My_Variables->F_Prop[focus].wmap != nullptr)) {
                if (save_wmap_project(save_path.c_str(), &My_Variables->F_Prop[focus])) {
                    My_Variables->F_Prop[focus].dirty = false;
                    add_recent_file(&usr_info, save_path.c_str());
                    // Register the save path so find_open_file() can detect
                    // this project is already open (prevents duplicate tabs
                    // when loading the same .wmap from recent files)
                    LF* fp = &My_Variables->F_Prop[focus];
                    strncpy(fp->Opened_File, save_path.c_str(), MAX_PATH - 1);
                    fp->Opened_File[MAX_PATH - 1] = '\0';
                    // Update tab name to the filename
                    char* slash = strrchr(fp->Opened_File, PLATFORM_SLASH);
                    fp->c_name = (slash != nullptr) ? slash + 1 : fp->Opened_File;
                }

                // Update default save path
                strncpy(usr_info.default_save_path, save_path.c_str(), MAX_PATH);
                char* ptr = strrchr(usr_info.default_save_path, '/');
#ifdef QFO2_WINDOWS
                char* bptr = strrchr(usr_info.default_save_path, '\\');
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
            if (resolve_path_icase(path.c_str(), "data/worldmap.txt", wmap_check, MAX_PATH)) {
                printf("  -> game root detected\n");
                strncpy(g_import_wmap_data_path, path.c_str(), MAX_PATH - 1);
                g_import_wmap_data_path[MAX_PATH - 1] = '\0';
                strncpy(g_import_wmap_base_name, "WRLDMP", 7);
                g_import_wmap_pending = true;
                found_root = true;
            }

            // Check 2: user selected the data/ folder itself -> worldmap.txt is
            // directly inside
            if (!found_root &&
                resolve_path_icase(path.c_str(), "worldmap.txt", wmap_check, MAX_PATH)) {
                // Strip trailing slashes, then strip the last path component
                // (the data/ dir) to get game root
                char parent[MAX_PATH];
                strncpy(parent, path.c_str(), MAX_PATH - 1);
                parent[MAX_PATH - 1] = '\0';
                int plen = static_cast<int>(strlen(parent));
                while (plen > 1 && (parent[plen - 1] == '/' || parent[plen - 1] == '\\')) {
                    parent[--plen] = '\0';
                }
                char* last_slash = strrchr(parent, '/');
#ifdef QFO2_WINDOWS
                char* last_bslash = strrchr(parent, '\\');
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

    // Create DAT2 Archive: step 1 — folder selected, open save dialog
    if (ifd::FileDialog::Instance().IsDone("CreateDAT2FolderDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            std::string path = ifd::FileDialog::Instance().GetResult().u8string();
            strncpy(g_create_dat2_folder_path, path.c_str(), MAX_PATH - 1);
            g_create_dat2_folder_path[MAX_PATH - 1] = '\0';
            g_create_dat2_folder_selected = true;
        }
        ifd::FileDialog::Instance().Close();
    }
    if (g_create_dat2_folder_selected) {
        g_create_dat2_folder_selected = false;
        init_IFD();
        ifd::FileDialog::Instance().Save("CreateDAT2SaveDialog", "Save DAT2 Archive",
                                         "DAT2 Archive (*.dat){.dat,.DAT}",
                                         usr_info.default_save_path);
    }

    // Create DAT2 Archive: step 2 — save path chosen, write archive
    if (ifd::FileDialog::Instance().IsDone("CreateDAT2SaveDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            std::string save_path = ifd::FileDialog::Instance().GetResult().u8string();
            auto collected = dat2::collect_from_directory(g_create_dat2_folder_path);
            if (collected.ok()) {
                dat2::Dat2WriteOptions opts;
                opts.compress = g_create_dat2_compress;
                auto ws = dat2::write_archive(save_path.c_str(), collected.value, opts);
                if (!ws.ok()) {
                    char msg[512];
                    snprintf(msg, sizeof(msg),
                             "[ERROR] Create DAT2 Archive\n\n"
                             "Failed to write archive:\n%s",
                             dat2::dat2_error_str(ws.error));
                    set_popup_warning(msg);
                }
            } else {
                char msg[512];
                snprintf(msg, sizeof(msg),
                         "[ERROR] Create DAT2 Archive\n\n"
                         "Failed to read directory:\n%s",
                         dat2::dat2_error_str(collected.error));
                set_popup_warning(msg);
            }
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
                printf("Import button clicked: path='%s' name='%s'\n", g_import_wmap_data_path,
                       g_import_wmap_base_name);
                g_import_error[0] = '\0';
                int msk_skipped = 0;
                LF* F_Prop = &My_Variables->F_Prop[*counter];
                bool ok =
                    import_wmap_from_fo2(g_import_wmap_data_path, g_import_wmap_base_name, F_Prop,
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

void ShowMainMenuBar(int* counter, struct variables* My_Variables) {
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
                ifd::FileDialog::Instance().Open("OpenWmapDialog", "Open Worldmap Project",
                                                 "Worldmap Project (*.wmap){.wmap,.WMAP}", false,
                                                 usr_info.default_load_path);
            }
            if (ImGui::MenuItem("Import Worldmap from FO2")) {
                init_IFD();
                ifd::FileDialog::Instance().Open(
                    "ImportWmapFolderDialog", "Select Fallout 2 data/ folder", "", false,
                    (usr_info.default_game_path[0] != 0) ? usr_info.default_game_path
                                                         : usr_info.default_load_path);
            }
            // File->Save: enabled only when focused window is a worldmap project
            {
                int focus = My_Variables->window_number_focus;
                bool can_save = (focus >= 0 && My_Variables->F_Prop[focus].wmap != nullptr);
                if (ImGui::MenuItem("Save Project", "Ctrl+S", false, can_save) && can_save) {
                    LF* fp = &My_Variables->F_Prop[focus];
                    if (fp->wmap->save_path[0] != '\0') {
                        save_wmap_project(fp->wmap->save_path, fp);
                        fp->dirty = false;
                    } else {
                        init_IFD();
                        ifd::FileDialog::Instance().Save("WmapSaveDialog", "Save Worldmap Project",
                                                         "Worldmap Project (*.wmap){.wmap,.WMAP}",
                                                         usr_info.default_save_path);
                    }
                }
            }
            ImGui::Separator();
            ImGui::MenuItem("Compress", nullptr, &g_create_dat2_compress);
            if (ImGui::MenuItem("Create DAT2 Archive...")) {
                init_IFD();
                ifd::FileDialog::Instance().Open("CreateDAT2FolderDialog", "Select Folder to Pack",
                                                 "", false, usr_info.default_load_path);
            }
            ImGui::Separator();
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
            if (ImGui::MenuItem("Cut", "CTRL+X", false, false)) {}
            if (ImGui::MenuItem("Copy", "CTRL+C", false, false)) {}
            if (ImGui::MenuItem("Paste", "CTRL+V", false, false)) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Config")) {
            if (ImGui::MenuItem("Reset ImGui.ini")) {
                g_reset_imgui_ini = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(g_reset_imgui_ini ? "Restart the application to apply the reset."
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

    // Escape shortcut to exit edit mode
    if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal)) {
        int focus = My_Variables->window_number_focus;
        if (focus >= 0 && My_Variables->F_Prop[focus].editing_enabled) {
            My_Variables->F_Prop[focus].wmap_edit_toggled = true;
        }
    }

    // Ctrl+S shortcut for saving worldmap projects
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) {
        int focus = My_Variables->window_number_focus;
        if (focus >= 0 && (My_Variables->F_Prop[focus].wmap != nullptr)) {
            LF* fp = &My_Variables->F_Prop[focus];
            if (fp->wmap->save_path[0] != '\0') {
                save_wmap_project(fp->wmap->save_path, fp);
                fp->dirty = false;
            } else {
                init_IFD();
                ifd::FileDialog::Instance().Save("WmapSaveDialog", "Save Worldmap Project",
                                                 "Worldmap Project (*.wmap){.wmap,.WMAP}",
                                                 usr_info.default_save_path);
            }
        }
    }
}

void main_window_bttns(variables* My_Variables, int* counter) {
    LF* F_Prop = &My_Variables->F_Prop[*counter];
    image_data* img_data = &F_Prop->img_data;

    bool success = ImDialog_load_files(F_Prop, img_data, &usr_info, &My_Variables->shaders,
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
            const char* full_path = usr_info.recent_files[i];
            const char* filename = strrchr(full_path, PLATFORM_SLASH);
            filename = (filename != nullptr) ? filename + 1 : full_path;

            if (ImGui::Selectable(filename)) {
                int existing = find_open_file(My_Variables->F_Prop, *counter, full_path);
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
                    F_Prop->file_open_window = File_Type_Check(F_Prop, &My_Variables->shaders,
                                                               &F_Prop->img_data, full_path);
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
