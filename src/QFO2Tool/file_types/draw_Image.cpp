#include "draw_Image.h"

#include "../City_Layer.h"
#include "../Edit_Animation.h"
#include "../Image2Texture.h"
#include "../Load_Files.h"
#include "../MSK_Convert.h"
#include "../Preview_Image.h"
#include "draw_common.h"
#include "imgui.h"

void toolbar_Image(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;
    image_data* edit_data = &F_Prop->edit_data;

    bool alpha_off = checkbox_handler("Alpha Enabled", &F_Prop->alpha);
    const char* items[] = {"Euclidan Color Matching", "Not Implemented..."};
    ImGui::SameLine();
    ImGui::Combo("##color_match", &My_Variables->color_match_algo, items, IM_ARRAYSIZE(items));

    if (!F_Prop->editing_enabled) {
        if (!F_Prop->palettized) {
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
                prep_image_SURFACE(F_Prop, My_Variables->FO_Palette, My_Variables->color_match_algo,
                                   &discard, alpha_off);
            }

            if (ImGui::Button("Convert Image to MSK")) {
                Convert_SURFACE_to_MSK(F_Prop->img_data.ANM_dir[0].frame_data[0], &F_Prop->img_data,
                                       0);
                prep_image_SURFACE(F_Prop, My_Variables->FO_Palette, My_Variables->color_match_algo,
                                   &F_Prop->editing_enabled, alpha_off);
            }
        }
    }

    if (img_data->ANM_dir[img_data->display_orient_num].num_frames > 1) {
        if (ImGui::Button("Convert Animation to FRM for Editing")) {
            F_Prop->show_image_render = crop_animation_SURFACE(
                img_data, edit_data, My_Variables->FO_Palette, 0, &My_Variables->shaders);
        }
    }

    // Enable/Disable Editing
    draw_edit_toggle(ctx);

    // Edit toolbar (Reset Image)
    draw_edit_toolbar(ctx);
}

void preview_Image(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;

    ImVec2 pre_cursor = ImGui::GetCursorScreenPos();
    Preview_Image(My_Variables, img_data, (F_Prop->show_stats || ctx->usr_info->show_image_stats));
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
