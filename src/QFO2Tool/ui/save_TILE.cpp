#include "save_TILE.h"

#include "../Load_Files.h"
#include "../Save_Files.h"
#include "../worldmap/City_Layer.h"
#include "../worldmap/Layer.h"
#include "../worldmap/Maps_Txt.h"
#include "imgui.h"

bool save_TILE_popup(LF* F_Prop, user_info* usr_info) {
    image_data* img_data = &F_Prop->edit_data;
    Save_Info* sv_info = {};

    bool open_window = true;
    // TODO: replace ImGui::Begin() with BeginPopupModal()?
    ImGui::Begin("Export Worldmap", &open_window);
    if (open_window) {
        int msk_i =
            find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count, LayerType::MSK);
        Surface* msk = (msk_i >= 0) ? F_Prop->img_data.overlay[msk_i].srfc : nullptr;
        const char* preset = (F_Prop->wmap != nullptr) ? "WRLDMP" : nullptr;
        int city_i =
            find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count, LayerType::CITY);
        city_layer_data* city = (city_i >= 0)
                                    ? (city_layer_data*)F_Prop->img_data.overlay[city_i].source_data
                                    : nullptr;
        int maps_i =
            find_overlay(F_Prop->img_data.overlay, F_Prop->img_data.overlay_count, LayerType::MAPS);
        maps_txt_data* maps =
            (maps_i >= 0) ? (maps_txt_data*)F_Prop->img_data.overlay[maps_i].source_data : nullptr;
        open_window =
            ImDialog_save_TILE_SURFACE(img_data, usr_info, sv_info, msk, preset, city, maps);
    }
    ImGui::End();

    return open_window;
}
