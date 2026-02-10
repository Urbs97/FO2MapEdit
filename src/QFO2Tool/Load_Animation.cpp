#include "Load_Animation.h"

#include "Image2Texture.h"
#include "platform_io.h"
#include "ui/ImGui_Warning.h"

#include <algorithm>
#include <cstdio>

bool Drag_Drop_Load_Animation(std::vector<std::filesystem::path>& path_set, LF* F_Prop) {
    char buffer[MAX_PATH];
    char folder[MAX_PATH];
    image_data* img_data = &F_Prop->img_data;
    std::sort(path_set.begin(), path_set.end());
    int num_frames = static_cast<int>(path_set.size());

    // folder name directions are NE/E/SE/SW/W/NW
    snprintf(folder, MAX_PATH, "%s",
             (*path_set.begin()).parent_path().filename().u8string().c_str());
    Direction dir = assign_direction(folder);

    // store filepaths in this directory for navigating through
    snprintf(F_Prop->Opened_File, MAX_PATH, "%s",
             (*path_set.begin()).parent_path().parent_path().u8string().c_str());
    F_Prop->c_name = strrchr(F_Prop->Opened_File, PLATFORM_SLASH) + 1;
    F_Prop->extension = strrchr(F_Prop->Opened_File, '.') + 1;
    Next_Prev_File(F_Prop->Next_File, F_Prop->Prev_File, F_Prop->Frst_File, F_Prop->Last_File,
                   F_Prop->Opened_File);

    if (img_data->ANM_dir == nullptr) {
        img_data->ANM_dir = (ANM_Dir*)malloc(sizeof(ANM_Dir) * 6);
        if (img_data->ANM_dir == nullptr) {
            // TODO: log out to txt file
            set_popup_warning("[ERROR] Drag_Drop_Load_Animation()\n\n"
                              "Unable to allocate enough memory.");
            printf("Unable to allocate enough memory : L%d\n", __LINE__);
            return false;
        }
        new (img_data->ANM_dir) ANM_Dir[6];
    }

    for (int i = 0; i < 6; i++) {
        if (img_data->ANM_dir[i].frame_box == nullptr) {
            img_data->ANM_dir[i].frame_box = (rectangle*)calloc(1, sizeof(rectangle));
        }
        if (img_data->ANM_dir[i].frame_box == nullptr) {
            // TODO: log to file
            set_popup_warning("[ERROR] Drag_Drop_Load_Animation()\n\n"
                              "Unable to allocate memory for ANM_dir[i].frame_box.");
            printf("Unable to allocate memory for ANM_dir[i].frame_box: %d", __LINE__);
            for (int j = 0; j < 6; j++) {
                if (img_data->ANM_dir[j].frame_box != nullptr) {
                    free(img_data->ANM_dir[j].frame_box);
                }
            }
            free(img_data->ANM_dir);
            return false;
        }
    }

    int dir_i = static_cast<int>(dir);
    Surface** frame_data = img_data->ANM_dir[dir_i].frame_data;
    if (frame_data != nullptr) {
        free(static_cast<void*>(frame_data));
        frame_data = nullptr;
    }
    frame_data = (Surface**)calloc(1, sizeof(Surface*) * num_frames);
    if (frame_data == nullptr) {
        // TODO: log out to txt file
        set_popup_warning("[ERROR] Drag_Drop_Load_Animation()\n\n"
                          "Unable to allocate enough memory for frame_data");
        printf("Unable to allocate enough memory for frame_data: L%d\n", __LINE__);
        return false;
    }
    img_data->ANM_dir[dir_i].frame_data = frame_data;
    img_data->ANM_dir[dir_i].orientation = dir;
    if (img_data->ANM_dir[dir_i].num_frames != num_frames) {
        img_data->ANM_dir[dir_i].num_frames = num_frames;
    }

    // iterate over images in directory provided and assign to frame_data[]
    int i = 0;
    for (const std::filesystem::path& path : path_set) {
        frame_data[i] = Load_File_to_RGBA(path.u8string().c_str());
        if (frame_data[i] != nullptr) {
            // only increment if current frame_data[] has been filled
            i++;
        }
    }
    if (frame_data[0] == nullptr) {
        // nothing could be loaded in this folder
        free(static_cast<void*>(frame_data));
        return false;
    }

    F_Prop->img_data.type = img_type::OTHER;
    // TODO: refactor img_data.width/height out in favor of FRM_boundary_box?
    img_data->width = frame_data[0]->w;
    img_data->height = frame_data[0]->h;
    img_data->display_orient_num = dir_i;

    Surface* srfc = img_data->ANM_dir[dir_i].frame_data[0];

    img_data->FRM_texture = init_texture(srfc, srfc->w, srfc->h, img_type::OTHER);

    bool success = framebuffer_init(&img_data->render_texture, &img_data->framebuffer,
                                    img_data->width, img_data->height);
    if (!success) {
        // TODO: log out to txt file
        set_popup_warning("[ERROR] Drag_Drop_Load_Animation()\n\n"
                          "Image framebuffer failed to attach correctly?\n");
        printf("Image framebuffer failed to attach correctly: L%d\n", __LINE__);
        return false;
    }
    return true;

    // prep_extension(F_Prop, NULL, (char*)(*path_set.begin()).parent_path().u8string().c_str());
    // return true;
}

// If folder name is not matched, default NE is assigned
Direction assign_direction(char* direction) {
    if (strncmp(direction, "NE\0", sizeof("NE\0")) == 0) {
        return Direction::NE;
    }
    if (strncmp(direction, "E\0", sizeof("E\0")) == 0) {
        return Direction::E;
    }
    if (strncmp(direction, "SE\0", sizeof("SE\0")) == 0) {
        return Direction::SE;
    }
    if (strncmp(direction, "SW\0", sizeof("SW\0")) == 0) {
        return Direction::SW;
    }
    if (strncmp(direction, "W\0", sizeof("W\0")) == 0) {
        return Direction::W;
    }
    if (strncmp(direction, "NW\0", sizeof("NW\0")) == 0) {
        return Direction::NW;
    }
    // default
    return Direction::NE;
}

void set_directions(const char** names_array, image_data* img_data) {
    Direction* dir_ptr = nullptr;

    for (int i = 0; i < 6; i++) {
        dir_ptr = &img_data->ANM_dir[i].orientation;
        assert(dir_ptr != nullptr && "Not FRM or OTHER?");
        switch (*dir_ptr) {
            case (Direction::NE):
                names_array[i] = "NE";
                break;
            case (Direction::E):
                names_array[i] = "E";
                break;
            case (Direction::SE):
                names_array[i] = "SE";
                break;
            case (Direction::SW):
                names_array[i] = "SW";
                break;
            case (Direction::W):
                names_array[i] = "W";
                break;
            case (Direction::NW):
                names_array[i] = "NW";
                break;
            default:
                names_array[i] = "no image";
                break;
        }
    }
}

void Clear_img_data(image_data* img_data) {
    clear_all_overlays(img_data->overlay, &img_data->overlay_count);
    if (img_data->FRM_data != nullptr) {
        free(img_data->FRM_data);
        img_data->FRM_data = nullptr;
        img_data->FRM_hdr = nullptr;
    } else if (img_data->FRM_hdr != nullptr) {
        // FRM_hdr allocated separately (OTHER path in prep_image_SURFACE)
        free(img_data->FRM_hdr);
        img_data->FRM_hdr = nullptr;
    }
    if (img_data->ANM_dir != nullptr) {
        for (int i = 0; i < 6; i++) {
            if (img_data->ANM_dir[i].frame_data != nullptr) {
                // TODO: check if number of frames are set for individual images
                for (int j = 0; j < img_data->ANM_dir[i].num_frames; j++) {
                    FreeSurface(img_data->ANM_dir[i].frame_data[j]);
                }
                free(static_cast<void*>(img_data->ANM_dir[i].frame_data));
                img_data->ANM_dir[i].frame_data = nullptr;
            }
            free(img_data->ANM_dir[i].frame_box);
            img_data->ANM_dir[i].frame_box = nullptr;
        }
        free(img_data->ANM_dir);
        img_data->ANM_dir = nullptr;
    }
    img_data->type = img_type::UNK;
}

void Gui_Video_Controls(image_data* img_data, img_type type) {
    ImVec2 origin = ImGui::GetCursorPos();
    // position buttons based on bottom right edge of window
    ImVec2 wind_pos = ImGui::GetWindowSize();
    ImVec2 scrl_pos;
    // TODO: might need scrl_pos.x in the future?
    // scrl_pos.x = ImGui::GetScrollX();
    scrl_pos.y = ImGui::GetScrollY();

    // check if this image has multiple frames or directions
    int num_frames = img_data->ANM_dir[img_data->display_orient_num].num_frames;
    bool has_multiple_dirs = false;
    for (int i = 0; i < 6; i++) {
        if (img_data->ANM_dir[i].num_frames > 0 && i != img_data->display_orient_num) {
            has_multiple_dirs = true;
            break;
        }
    }
    bool is_animation = (num_frames > 1) || has_multiple_dirs;

    int max_frame = 0;
    if (is_animation) {
        // gui video controls
        ImGui::SetCursorPosY(wind_pos.y + scrl_pos.y - 80);
        const char* speeds[] = {"Pause", "1/4x", "1/2x", "1x", "2x"};
        ImGui::SliderInt("Playback Speed", &img_data->playback_speed, 0, 4,
                         speeds[img_data->playback_speed]);

        if (has_multiple_dirs) {
            const char* directions[6];
            set_directions(directions, img_data);
            ImGui::SliderInt("Direction", &img_data->display_orient_num, 0, 5,
                             directions[img_data->display_orient_num]);
        }

        if (ImGui::IsWindowFocused()) {
            if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
                static int last_selected_speed =
                    3; // 3 is index value for 1.0x speed in playback_speeds[]
                if (img_data->playback_speed == 0) {
                    img_data->playback_speed = last_selected_speed;
                } else {
                    last_selected_speed = img_data->playback_speed;
                    img_data->playback_speed = 0;
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
                img_data->display_frame_num++;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
                img_data->display_frame_num--;
            }
            if (has_multiple_dirs) {
                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                    img_data->display_orient_num++;
                    if (img_data->display_orient_num > 5) {
                        img_data->display_orient_num = 0;
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                    img_data->display_orient_num--;
                    if (img_data->display_orient_num < 0) {
                        img_data->display_orient_num = 5;
                    }
                }
            }
        }

        if (num_frames > 0) {
            max_frame = num_frames - 1;
        }
        ImGui::SliderInt("Frame Number", &img_data->display_frame_num, 0, max_frame, nullptr);
    }

    if (img_data->display_frame_num > max_frame) {
        img_data->display_frame_num = max_frame;
    } else if (img_data->display_frame_num < 0) {
        img_data->display_frame_num = 0;
    }

    ImGui::SetCursorPos(origin);
}
