#include "draw_common.h"

#include "../Image2Texture.h"
#include "../Load_Files.h"
#include "../Save_Files.h"
#include "../rendering/display_FRM_OpenGL.h"
#include "Edit_Image.h"
#include "imgui.h"

// "Export as PNG" button + popup — used by FRM and MSK
void draw_export_png_button(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    image_data* img_data = &F_Prop->img_data;

    if (F_Prop->img_data.ANM_dir == nullptr) {
        ImGui::BeginDisabled();
    }
    {
        char png_popup_id[32];
        snprintf(png_popup_id, sizeof(png_popup_id), "Export as PNG##%02d", ctx->counter);
        if (ImGui::Button("Export as PNG")) {
            ImGui::OpenPopup(png_popup_id);
        }
        bool open = true;
        if (ImGui::BeginPopupModal(png_popup_id, &open)) {
            open = save_PNG_popup_INTERNAL(img_data, ctx->usr_info);
            ImGui::EndPopup();
        }
    }
    if (F_Prop->img_data.ANM_dir == nullptr) {
        ImGui::EndDisabled();
    }
}

// Enable/Disable Editing buttons — used by FRM and Image (non-wmap, non-MSK)
void draw_edit_toggle(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;
    ANM_Dir(&edit_struct)[6] = F_Prop->edit_struct;

    if (!F_Prop->editing_enabled) {
        if (ImGui::Button("Enable Editing")) {
            if (img_data->type == img_type::FRM) {
                prep_image_SURFACE(F_Prop, My_Variables->FO_Palette, My_Variables->color_match_algo,
                                   &F_Prop->editing_enabled, false);
            }
            F_Prop->editing_enabled = true;
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
                        Surface* src = F_Prop->edit_data.ANM_dir[d].frame_data[f];
                        Surface* dst = F_Prop->img_data.ANM_dir[d].frame_data[f];
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

// Reset Image button + layer panel — shown when editing is enabled
void draw_edit_toolbar(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    if (!F_Prop->editing_enabled) {
        return;
    }

    image_data* ed = &F_Prop->edit_data;
    ANM_Dir(&edit_struct)[6] = F_Prop->edit_struct;
    StrokeState& stroke_state = F_Prop->stroke_state;

    if (ImGui::Button("Reset Image")) {
        stroke_state_cleanup(&stroke_state);
        int num = ed->display_frame_num;
        int dir = ed->display_orient_num;
        bool editing_overlay =
            (F_Prop->active_layer >= 0 && F_Prop->active_layer < F_Prop->img_data.overlay_count);
        Surface* edit_srfc = nullptr;
        if (!editing_overlay) {
            if (edit_struct[dir].frame_data != nullptr) {
                edit_srfc = edit_struct[dir].frame_data[num];
            }
        } else {
            edit_srfc = F_Prop->img_data.overlay[F_Prop->active_layer].edit_srfc;
        }
        if (edit_srfc != nullptr) {
            ClearSurface(edit_srfc);
            Surface* src = nullptr;
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

// Layer panel in edit toolbar — WMAP only
void draw_layer_panel_toolbar(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    if (!F_Prop->editing_enabled) {
        return;
    }
    if (F_Prop->wmap == nullptr) {
        return;
    }
    shader_info* shaders = &ctx->vars->shaders;
    draw_layer_panel(F_Prop, shaders, &F_Prop->img_data);
}
