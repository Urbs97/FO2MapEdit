#include "doctest.h"
#include "test_helpers.h"
#include "tiles_pattern.h"

#include <cstring>

TEST_CASE("assign_tile_id single tile match") {
    // TILES.LST: each entry ends with \n, line counting starts at 0
    // and increments at each \n
    const char* tiles_lst = "tile_000.FRM\n"
                            "tile_001.FRM\n"
                            "tile_002.FRM\n";

    tt_arr_handle* h = make_test_handle(1);
    strncpy(h->tile[0].name_ptr, "tile_001.FRM", 13);
    h->tile[0].tile_id = 0;

    assign_tile_id(h, tiles_lst);

    // "tile_001.FRM" appears after the 2nd \n, so current_line = 2
    CHECK(h->tile[0].tile_id == 2);

    free(h);
}

TEST_CASE("assign_tile_id blank tiles skipped") {
    const char* tiles_lst = "tile_000.FRM\n";

    tt_arr_handle* h = make_test_handle(2);
    strncpy(h->tile[0].name_ptr, "tile_000.FRM", 13);
    h->tile[0].tile_id = 0;
    // tile_id == (uint32_t)-1 means blank, should be skipped
    h->tile[1].tile_id = (uint32_t)-1;

    assign_tile_id(h, tiles_lst);

    CHECK(h->tile[0].tile_id == 1);
    CHECK(h->tile[1].tile_id == (uint32_t)-1);

    free(h);
}

TEST_CASE("assign_tile_id multiple tiles all found") {
    const char* tiles_lst = "aaa000.FRM\n"
                            "bbb000.FRM\n"
                            "ccc000.FRM\n";

    tt_arr_handle* h = make_test_handle(3);
    strncpy(h->tile[0].name_ptr, "aaa000.FRM", 13);
    h->tile[0].tile_id = 0;
    strncpy(h->tile[1].name_ptr, "bbb000.FRM", 13);
    h->tile[1].tile_id = 0;
    strncpy(h->tile[2].name_ptr, "ccc000.FRM", 13);
    h->tile[2].tile_id = 0;

    assign_tile_id(h, tiles_lst);

    CHECK(h->tile[0].tile_id == 1);
    CHECK(h->tile[1].tile_id == 2);
    CHECK(h->tile[2].tile_id == 3);

    free(h);
}

TEST_CASE("assign_tile_id tile not found stays at 0") {
    const char* tiles_lst = "aaa000.FRM\n";

    tt_arr_handle* h = make_test_handle(1);
    strncpy(h->tile[0].name_ptr, "zzz999.FRM", 13);
    h->tile[0].tile_id = 0;

    assign_tile_id(h, tiles_lst);

    // not found, tile_id remains 0
    CHECK(h->tile[0].tile_id == 0);

    free(h);
}

TEST_CASE("assign_tile_id empty TILES.LST") {
    const char* tiles_lst = "";

    tt_arr_handle* h = make_test_handle(1);
    strncpy(h->tile[0].name_ptr, "tile_000.FRM", 13);
    h->tile[0].tile_id = 0;

    assign_tile_id(h, tiles_lst);

    CHECK(h->tile[0].tile_id == 0);

    free(h);
}
