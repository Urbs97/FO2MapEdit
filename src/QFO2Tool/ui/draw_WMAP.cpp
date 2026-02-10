#include "draw_WMAP.h"

#include "../Image2Texture.h"
#include "../Load_Files.h"
#include "../rendering/display_FRM_OpenGL.h"
#include "../worldmap/City_Layer.h"
#include "Edit_Image.h"
#include "Preview_Image.h"
#include "draw_common.h"
#include "imgui.h"

void toolbar_WMAP(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;
    ANM_Dir(&edit_struct)[6] = F_Prop->edit_struct;
    Palette* pxlFMT_FO_Pal = My_Variables->FO_Palette;

    if (!F_Prop->palettized) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Export Worldmap")) {
        F_Prop->show_squares = true;
        F_Prop->show_tiles = false;
        // Ensure edit_data is initialized before export
        if (F_Prop->edit_data.ANM_dir == nullptr) {
            prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal, My_Variables->color_match_algo, nullptr,
                               false);
        }
        commit_map_edits(edit_struct, &F_Prop->edit_data);
        commit_all_overlay_edits(&F_Prop->img_data);
        F_Prop->edit_data.type = img_type::TILE;
        image_data* ed = &F_Prop->edit_data;
        int dir = ed->display_orient_num;
        animate_SURFACE_to_sub_texture(ed, ed->ANM_dir[dir].frame_data[0],
                                       My_Variables->CurrentTime_ms);
        shader_info* shaders = &My_Variables->shaders;
        draw_PAL_to_framebuffer(shaders->FO_pal, shaders->render_PAL_shader,
                                &shaders->giant_triangle, ed, F_Prop->img_data.overlay,
                                F_Prop->img_data.overlay_count);
        F_Prop->open_export_popup = true;
    }
    if (F_Prop->open_export_popup) {
        const FileTypeEntry* wmap_entry = find_file_type("WMAP");
        if (wmap_entry != nullptr && (wmap_entry->flags & FileTypeFlags::HAS_EXPORT)) {
            F_Prop->open_export_popup = wmap_entry->save_popup(F_Prop, ctx->usr_info);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Town-Map Tiles")) {
        F_Prop->show_squares = false;
        F_Prop->show_tiles = true;
        prep_image_SURFACE(F_Prop, pxlFMT_FO_Pal, My_Variables->color_match_algo,
                           &F_Prop->preview_tiles_window, false);
        F_Prop->show_image_render = false;
    }
    if (!F_Prop->palettized) {
        ImGui::EndDisabled();
    }

    // Export as PNG
    draw_export_png_button(ctx);

    // Layer panel + Reset Image (edit mode)
    draw_layer_panel_toolbar(ctx);
    draw_edit_toolbar(ctx);
}

void preview_WMAP(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    variables* My_Variables = ctx->vars;
    image_data* img_data = &F_Prop->img_data;

    // WMAP loads as img_type::FRM (palette-indexed), so use the FRM preview path
    ImVec2 pre_cursor = ImGui::GetCursorScreenPos();
    preview_FRM_SURFACE(My_Variables, img_data,
                        (F_Prop->show_stats || ctx->usr_info->show_image_stats));
    // Allow clicking city markers in preview mode
    int city_idx = find_overlay(img_data->overlay, img_data->overlay_count, LayerType::CITY);
    if (city_idx >= 0 && img_data->overlay[city_idx].visible) {
        ImVec2 img_pos = {pre_cursor.x + img_data->offset.x, pre_cursor.y + img_data->offset.y};
        Edit_City_Layer(My_Variables, img_pos, img_data, img_data, city_idx);
    }

    Gui_Video_Controls(img_data, img_data->type);
}
