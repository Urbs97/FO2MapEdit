#include "draw_INT.h"

#include "../Load_Files.h"
#include "imgui.h"

void preview_INT(DrawContext* ctx) {
    LF* F_Prop = ctx->F_Prop;
    if (F_Prop->ssl_text == nullptr) {
        ImGui::Text("No decompiled text available.");
        return;
    }
    ImGui::InputTextMultiline("##ssl_source", F_Prop->ssl_text, F_Prop->ssl_text_len + 1,
                              ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly);
}
