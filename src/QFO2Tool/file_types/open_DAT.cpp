#include "open_DAT.h"

#include "../Load_Files.h"
#include "../dat2/dat2_tree_view.h"

bool open_DAT(LF* F_Prop, shader_info* shaders, image_data* img_data) {
    (void)shaders;
    (void)img_data;
    F_Prop->file_open_window = load_dat_archive(F_Prop);
    return F_Prop->file_open_window;
}
