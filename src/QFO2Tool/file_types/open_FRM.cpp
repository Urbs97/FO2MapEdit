#include "open_FRM.h"

#include "../Load_Files.h"
#include "../load_FRM_OpenGL.h"

bool open_FRM(LF* F_Prop, shader_info* shaders, image_data* img_data) {
    F_Prop->file_open_window = load_FRM_OpenGL(F_Prop->Opened_File, img_data, shaders);
    if (!F_Prop->file_open_window) {
        return false;
    }
    img_data->type = img_type::FRM;
    return true;
}
