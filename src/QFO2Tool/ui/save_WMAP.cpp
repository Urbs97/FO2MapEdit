#include "save_WMAP.h"

#include "../Load_Files.h"
#include "../worldmap/Worldmap_Project.h"

bool quicksave_WMAP(LF* F_Prop, user_info* /* usr_info */) {
    if (F_Prop->wmap == nullptr || F_Prop->wmap->save_path[0] == '\0') {
        return false;
    }
    save_wmap_project(F_Prop->wmap->save_path, F_Prop);
    return true;
}
