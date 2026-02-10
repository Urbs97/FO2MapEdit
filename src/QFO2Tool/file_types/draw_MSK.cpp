#include "draw_MSK.h"

#include "../Image2Texture.h"
#include "../Load_Files.h"
#include "../Preview_Image.h"
#include "draw_common.h"
#include "imgui.h"

void toolbar_MSK(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;

    if (!F_Prop->editing_enabled) {
        if (ImGui::Button("Edit MSK file")) {
            prep_image_SURFACE(F_Prop, My_Variables->FO_Palette, My_Variables->color_match_algo,
                               &F_Prop->editing_enabled, false);
            // MSK files: set active layer to the MSK overlay
            int msk_idx = find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count,
                                       LayerType::MSK);
            F_Prop->active_layer = msk_idx;
        }
    }

    // Export as PNG
    draw_export_png_button(ctx);

    // Edit toolbar (Reset Image)
    draw_edit_toolbar(ctx);
}

void preview_MSK(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;

    Preview_MSK_Image(My_Variables, img_data,
                      (F_Prop->show_stats || ctx->usr_info->show_image_stats));
}
