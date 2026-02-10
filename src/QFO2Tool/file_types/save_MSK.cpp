#include "save_MSK.h"

#include "../Load_Files.h"
#include "../Save_Files.h"
#include "imgui.h"

bool save_MSK_popup(LF* F_Prop, user_info* usr_info) {
    image_data* img_data = &F_Prop->img_data;
    Save_Info* sv_info = nullptr;

    bool open_window = true;
    // TODO: replace ImGui::Begin() with BeginPopupModal()?
    ImGui::Begin("Export MSK", &open_window);
    if (open_window) {
        open_window = ImDialog_save_TILE_SURFACE(img_data, usr_info, sv_info);
    }
    ImGui::End();

    return open_window;
}
