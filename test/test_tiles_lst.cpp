#include "Edit_TILES_LST.h"
#include "doctest.h"
#include "test_helpers.h"

#include <cstring>

TEST_CASE("set_false resets all flags") {
    export_state state;
    state.auto_export = true;
    state.export_proto = true;
    state.export_pattern = true;
    state.chk_game_path = true;
    state.make_FRM_LST = true;
    state.make_PRO_LST = true;
    state.make_PRO_MSG = true;
    state.load_files = true;
    state.append_FRM_LST = true;
    state.append_PRO_LST = true;
    state.append_PRO_MSG = true;

    set_false(&state);

    CHECK(state.auto_export == false);
    CHECK(state.export_proto == false);
    CHECK(state.export_pattern == false);
    CHECK(state.chk_game_path == false);
    CHECK(state.make_FRM_LST == false);
    CHECK(state.make_PRO_LST == false);
    CHECK(state.make_PRO_MSG == false);
    CHECK(state.load_files == false);
    CHECK(state.append_FRM_LST == false);
    CHECK(state.append_PRO_LST == false);
    CHECK(state.append_PRO_MSG == false);
}

TEST_CASE("make_name_list_arr parses names") {
    // make_name_list_arr expects newline-delimited names
    // and walks until '\0'
    char input[] = "tile_000.FRM\n"
                   "tile_001.FRM\n"
                   "tile_002.FRM\n";

    tile_name_arr* arr = make_name_list_arr(input);

    // 3 entries: indices 0, 1, 2
    CHECK(arr[0].name_ptr == input);
    CHECK(arr[0].length == strlen("tile_000.FRM"));
    CHECK(arr[0].next == 1);

    CHECK(arr[1].name_ptr == input + 13); // after "tile_000.FRM\n"
    CHECK(arr[1].length == strlen("tile_001.FRM"));
    CHECK(arr[1].next == 2);

    // last entry has next == 0 (sentinel)
    CHECK(arr[2].name_ptr == input + 26);
    CHECK(arr[2].length == strlen("tile_002.FRM"));
    CHECK(arr[2].next == 0);

    free(arr);
}

TEST_CASE("make_name_list_arr single entry") {
    char input[] = "only_one.FRM\n";

    tile_name_arr* arr = make_name_list_arr(input);

    CHECK(arr[0].name_ptr == input);
    CHECK(arr[0].length == strlen("only_one.FRM"));
    CHECK(arr[0].next == 0); // sentinel for last entry

    free(arr);
}

TEST_CASE("make_FRM_tile_LST happy path NULL match_buff") {
    // NULL match_buff means all tiles are included
    tt_arr_handle* h = make_test_handle(2);
    strncpy(h->tile[0].name_ptr, "tile_000.FRM", 13);
    h->tile[0].tile_id = 0;
    strncpy(h->tile[1].name_ptr, "tile_001.FRM", 13);
    h->tile[1].tile_id = 0;

    char* result = make_FRM_tile_LST(h, nullptr);
    REQUIRE(result != nullptr);

    // each name gets \r\n appended
    CHECK(strstr(result, "tile_000.FRM\r\n") != nullptr);
    CHECK(strstr(result, "tile_001.FRM\r\n") != nullptr);

    // Side effect: tile_id is modified in-place to sequential tile_num
    CHECK(h->tile[0].tile_id == 0);
    CHECK(h->tile[1].tile_id == 1);

    free(result);
    free(h);
}

TEST_CASE("make_FRM_tile_LST skips tile_id -1") {
    tt_arr_handle* h = make_test_handle(3);
    strncpy(h->tile[0].name_ptr, "tile_000.FRM", 13);
    h->tile[0].tile_id = 0;
    // tile 1 is blank
    h->tile[1].tile_id = (uint32_t)-1;
    strncpy(h->tile[2].name_ptr, "tile_002.FRM", 13);
    h->tile[2].tile_id = 0;

    char* result = make_FRM_tile_LST(h, nullptr);
    REQUIRE(result != nullptr);

    CHECK(strstr(result, "tile_000.FRM\r\n") != nullptr);
    CHECK(strstr(result, "tile_002.FRM\r\n") != nullptr);
    // blank tile should not appear
    size_t len = strlen(result);
    // 2 tiles * (12 name + 2 \r\n) = 28
    CHECK(len == 28);

    free(result);
    free(h);
}

TEST_CASE("make_FRM_tile_LST match_buff bitmask") {
    tt_arr_handle* h = make_test_handle(2);
    strncpy(h->tile[0].name_ptr, "tile_000.FRM", 13);
    h->tile[0].tile_id = 0;
    strncpy(h->tile[1].name_ptr, "tile_001.FRM", 13);
    h->tile[1].tile_id = 0;

    // match_buff bit 0 set = skip tile 0
    uint8_t match_buff[1] = {0x01};

    char* result = make_FRM_tile_LST(h, match_buff);
    REQUIRE(result != nullptr);

    CHECK(strstr(result, "tile_000.FRM") == nullptr);
    CHECK(strstr(result, "tile_001.FRM\r\n") != nullptr);

    free(result);
    free(h);
}

TEST_CASE("check_FRM_LST_names duplicates detected") {
    // With auto_export=true, duplicates are excluded from output
    char old_lst[] = "tile_000.FRM\n"
                     "tile_001.FRM\n";

    tt_arr_handle* h = make_test_handle(2);
    strncpy(h->tile[0].name_ptr, "tile_000.FRM", 13);
    h->tile[0].tile_id = 0;
    strncpy(h->tile[1].name_ptr, "new_tile.FRM", 13);
    h->tile[1].tile_id = 0;

    export_state state = {};
    state.auto_export = true;

    char* result = check_FRM_LST_names(old_lst, h, &state);
    REQUIRE(result != nullptr);

    // tile_000.FRM is a duplicate, so only new_tile.FRM should be in result
    CHECK(strstr(result, "new_tile.FRM\r\n") != nullptr);
    CHECK(strstr(result, "tile_000.FRM") == nullptr);

    // tile_000.FRM should have been assigned its line number
    CHECK(h->tile[0].tile_id == 1);

    free(result);
    free(h);
}

TEST_CASE("check_FRM_LST_names no duplicates") {
    char old_lst[] = "existing.FRM\n";

    tt_arr_handle* h = make_test_handle(2);
    strncpy(h->tile[0].name_ptr, "new_one0.FRM", 13);
    h->tile[0].tile_id = 0;
    strncpy(h->tile[1].name_ptr, "new_one1.FRM", 13);
    h->tile[1].tile_id = 0;

    export_state state = {};
    state.auto_export = true;

    char* result = check_FRM_LST_names(old_lst, h, &state);
    REQUIRE(result != nullptr);

    CHECK(strstr(result, "new_one0.FRM\r\n") != nullptr);
    CHECK(strstr(result, "new_one1.FRM\r\n") != nullptr);

    free(result);
    free(h);
}

TEST_CASE("append_FRM_tiles_LST appends new") {
    char* old_lst = (char*)malloc(32);
    strcpy(old_lst, "existing.FRM\n");

    tt_arr_handle* h = make_test_handle(1);
    strncpy(h->tile[0].name_ptr, "brand_new.FRM", 14);
    h->tile[0].tile_id = 0;

    export_state state = {};
    state.auto_export = true;

    char* result = append_FRM_tiles_LST(old_lst, h, &state);
    REQUIRE(result != nullptr);
    CHECK(result != old_lst); // new buffer allocated

    CHECK(strstr(result, "existing.FRM\n") != nullptr);
    CHECK(strstr(result, "brand_new.FRM\r\n") != nullptr);

    free(result);
    free(old_lst);
    free(h);
}

// NOTE: "append_FRM_tiles_LST all duplicates" test omitted because
// when all tiles are duplicates, make_FRM_tile_LST() calls
// ImGui::OpenPopup() which crashes without an active ImGui context.
