#include "open_WMAP.h"

#include "../Load_Files.h"
#include "../Worldmap_Project.h"

bool open_WMAP(LF* F_Prop, shader_info* shaders, image_data* img_data) {
    F_Prop->file_open_window = load_wmap_project(F_Prop->Opened_File, F_Prop, img_data, shaders);
    return F_Prop->file_open_window;
}
