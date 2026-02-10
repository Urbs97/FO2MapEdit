#include "open_Image.h"

#include "../ImGui_Warning.h"
#include "../Image2Texture.h"
#include "../Load_Files.h"
#include "../MiniSDL.h"
#include "../load_FRM_OpenGL.h"

#include <cstdio>
#include <cstdlib>

bool open_Image(LF* F_Prop, shader_info* shaders, image_data* img_data) {
    (void)shaders;

    Surface* temp_surface = Load_File_to_RGBA(F_Prop->Opened_File);
    if (temp_surface == nullptr) {
        // TODO: log to file
        set_popup_warning("[ERROR] File_Type_Check()\n\n"
                          "Unable to load image.");
        printf("Unable to load image: %s\n", F_Prop->Opened_File);
        return false;
    }

    img_data->ANM_dir = (ANM_Dir*)malloc(sizeof(ANM_Dir) * 6);
    if (img_data->ANM_dir == nullptr) {
        // TODO: log to file
        set_popup_warning("[ERROR] File_Type_Check()\n\n"
                          "Unable to allocate memory for ANM_dir.");
        printf("Unable to allocate memory for ANM_dir: %d\n", __LINE__);
        return false;
    }
    // initialize allocated memory
    new (img_data->ANM_dir) ANM_Dir[6];

    img_data->ANM_dir[0].frame_data = (Surface**)malloc(sizeof(Surface*));
    if (img_data->ANM_dir[0].frame_data == nullptr) {
        // TODO: log to file
        set_popup_warning("[ERROR] File_Type_Check()\n\n"
                          "Unable to allocate memory for ANM_Frame.");
        printf("Unable to allocate memory for ANM_Frame: %d\n", __LINE__);
        free(img_data->ANM_dir);
        img_data->ANM_dir = nullptr;
        return false;
    }
    Surface* srfc = img_data->ANM_dir[0].frame_data[0] = temp_surface;
    if (img_data->ANM_dir->frame_data != nullptr) {
        img_data->width = srfc->w;
        img_data->height = srfc->h;
        img_data->ANM_dir[0].num_frames = 1;

        img_data->type = img_type::OTHER;

        img_data->FRM_texture = init_texture(srfc, srfc->w, srfc->h, img_data->type);

        framebuffer_init(&img_data->render_texture, &img_data->framebuffer, srfc->w, srfc->h);

        // assign display direction to same as image slot
        // so we can see the image on load
        img_data->display_orient_num = static_cast<int>(Direction::NE);
        img_data->display_frame_num = 0;

        F_Prop->file_open_window = true;
    }

    if (img_data->ANM_dir[0].frame_box == nullptr) {
        img_data->ANM_dir[0].frame_box = (rectangle*)calloc(1, sizeof(rectangle));
    }
    if (img_data->ANM_dir[0].frame_box == nullptr) {
        // TODO: log to file
        set_popup_warning("[ERROR] File_Type_Check()\n\n"
                          "Unable to allocate memory for ANM_dir[0].frame_box.");
        printf("Unable to allocate memory for ANM_dir[0].frame_box: %d\n", __LINE__);
        free(img_data->ANM_dir);
        img_data->ANM_dir = nullptr;
        return false;
    }
    return true;
}
