#include "draw_FRM.h"

#include "../Image2Texture.h"
#include "../Load_Files.h"
#include "../Load_Settings.h"
#include "../Save_Files.h"
#include "../worldmap/City_Layer.h"
#include "Edit_Image.h"
#include "Preview_Image.h"
#include "draw_common.h"
#include "imgui.h"

void toolbar_FRM(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;
    ANM_Dir(&edit_struct)[6] = F_Prop->edit_struct;

    // Export as PNG
    draw_export_png_button(ctx);

    // Enable/Disable Editing
    draw_edit_toggle(ctx);

    // Edit toolbar (Reset Image)
    draw_edit_toolbar(ctx);

    // Export FRM button + popup
    if (F_Prop->wmap == nullptr) {
        image_data* ed = &F_Prop->edit_data;
        if (ImGui::Button("Export FRM")) {
            if (ed->ANM_dir == nullptr) {
                prep_image_SURFACE(F_Prop, My_Variables->FO_Palette, My_Variables->color_match_algo,
                                   nullptr, false);
            }
            commit_map_edits(edit_struct, &F_Prop->edit_data);
            F_Prop->open_save_popup = true;
        }
        if (F_Prop->open_save_popup) {
            int msk_export_idx = find_overlay(F_Prop->img_data.overlay,
                                              F_Prop->img_data.overlay_count, LayerType::MSK);
            if (F_Prop->active_layer >= 0 && F_Prop->active_layer == msk_export_idx) {
                const FileTypeEntry* msk_entry = find_file_type("MSK");
                if ((msk_entry != nullptr) && (msk_entry->flags & FileTypeFlags::HAS_EXPORT)) {
                    F_Prop->open_save_popup = msk_entry->save_popup(F_Prop, ctx->usr_info);
                }
            } else {
                const FileTypeEntry* entry = find_file_type_by_img_type(ed->type);
                if ((entry != nullptr) && (entry->flags & FileTypeFlags::HAS_EXPORT)) {
                    F_Prop->open_save_popup = entry->save_popup(F_Prop, ctx->usr_info);
                }
            }
        }
    }
}

void preview_FRM(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;

    ImVec2 pre_cursor = ImGui::GetCursorScreenPos();
    preview_FRM_SURFACE(My_Variables, img_data,
                        (F_Prop->show_stats || ctx->usr_info->show_image_stats));
    // Allow clicking city markers in preview mode
    int city_idx = find_overlay(img_data->overlay, img_data->overlay_count, LayerType::CITY);
    if (city_idx >= 0 && img_data->overlay[city_idx].visible) {
        ImVec2 img_pos = {pre_cursor.x + img_data->offset.x, pre_cursor.y + img_data->offset.y};
        Edit_City_Layer(My_Variables, img_pos, img_data, img_data, city_idx);
    }

    // gui video controls
    Gui_Video_Controls(img_data, img_data->type);
}
