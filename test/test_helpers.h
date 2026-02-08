#pragma once

#include "town_map_tiles.h"

#include <cstdlib>

static tt_arr_handle* make_test_handle(int count) {
    auto* h = (tt_arr_handle*)calloc(1, sizeof(tt_arr_handle) + count * sizeof(tt_arr));
    h->size = count;
    return h;
}
