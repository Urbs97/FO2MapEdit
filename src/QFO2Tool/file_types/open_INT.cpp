#include "open_INT.h"

#include "../Load_Files.h"
#include "../ui/ImGui_Warning.h"
#include "int2ssl_wrapper.h"

#include <cstdlib>
#include <cstring>
#include <string>

bool open_INT(LF* F_Prop, shader_info* shaders, image_data* img_data) {
    (void)shaders;
    (void)img_data;

    std::string ssl_text;
    if (!decompile_int_to_ssl(F_Prop->Opened_File, ssl_text)) {
        set_popup_warning(ssl_text.c_str());
        return false;
    }

    // Free any previous text
    if (F_Prop->ssl_text != nullptr) {
        free(F_Prop->ssl_text);
    }

    F_Prop->ssl_text_len = ssl_text.size();
    F_Prop->ssl_text = (char*)malloc(F_Prop->ssl_text_len + 1);
    if (F_Prop->ssl_text == nullptr) {
        set_popup_warning("Error: out of memory allocating decompiled text");
        F_Prop->ssl_text_len = 0;
        return false;
    }
    memcpy(F_Prop->ssl_text, ssl_text.c_str(), F_Prop->ssl_text_len + 1);

    F_Prop->file_open_window = true;
    return true;
}
