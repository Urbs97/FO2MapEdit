#include "open_MSK.h"

#include "../Load_Files.h"
#include "../MSK_Convert.h"
#include "../display_FRM_OpenGL.h"
#include "../ui/ImGui_Warning.h"

#include <cstdio>

bool open_MSK(LF* F_Prop, shader_info* shaders, image_data* img_data) {
    F_Prop->file_open_window = Load_MSK_Tile_SURFACE(F_Prop->Opened_File, img_data);
    if (!F_Prop->file_open_window) {
        return false;
    }
    img_data->type = img_type::MSK;
    bool success =
        framebuffer_init(&img_data->render_texture, &F_Prop->img_data.framebuffer, 350, 300);
    if (!success) {
        // TODO: log to file
        set_popup_warning("[ERROR] Load_MSK_File_SURFACE\n\n"
                          "Image framebuffer failed to attach correctly?");
        printf("Image framebuffer failed to attach correctly?\n");
        return false;
    }
    int msk_idx = find_overlay(img_data->overlay, img_data->overlay_count, LayerType::MSK);
    if (msk_idx >= 0 && img_data->overlay[msk_idx].srfc != nullptr) {
        SURFACE_to_texture(img_data->overlay[msk_idx].srfc, img_data->overlay[msk_idx].texture, 350,
                           300, 1);
        draw_texture_to_framebuffer(shaders->FO_pal, shaders->render_FRM_shader,
                                    &shaders->giant_triangle, img_data->framebuffer,
                                    img_data->overlay[msk_idx].texture, 350, 300);
    }
    return true;
}
