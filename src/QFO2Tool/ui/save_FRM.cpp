#include "save_FRM.h"

#include "../Load_Files.h"
#include "../Save_Files.h"
#include "imgui.h"

bool save_FRM_popup(LF* F_Prop, user_info* usr_info) {
    image_data* img_data = &F_Prop->edit_data;

    Save_Info sv_info;
    bool open_window = true;
    // TODO: replace ImGui::Begin() with BeginPopupModal()?
    ImGui::Begin("Export FRM", &open_window);
    static int e;
    ImGui::RadioButton("Selected Frame", &e, 0);
    ImGui::RadioButton("Selected Direction", &e, 1);
    ImGui::RadioButton("All Directions", &e, 2);
    sv_info.s_type = (Save_Type)e;

    if (open_window) {
        open_window = ImDialog_save_FRM_SURFACE(img_data, usr_info, &sv_info);
    }
    ImGui::End();

    return open_window;
}

bool quicksave_FRM(LF* F_Prop, user_info* usr_info) {
    if (F_Prop->Opened_File[0] == '\0') {
        return false;
    }
    Save_Info sv_info;
    sv_info.s_type = Save_Type::all_dirs;
    save_FRM_SURFACE(F_Prop->Opened_File, &F_Prop->img_data, usr_info, &sv_info, true);
    return true;
}
