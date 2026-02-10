#include "doctest.h"
#include "test_helpers.h"
#include "worldmap/Proto_Files.h"

#include <cstring>

TEST_CASE("make_PRO_tile_MSG format") {
    proto_info info{};
    char name[] = "MyTile";
    char desc[] = "A cool tile";
    info.name = name;
    info.description = desc;

    char* result = make_PRO_tile_MSG(&info, 5);
    REQUIRE(result != nullptr);

    // tile_id=5 -> {500}{}{MyTile}\r\n{501}{}{A cool tile}\r\n
    CHECK(strstr(result, "{500}{}{MyTile}\r\n") != nullptr);
    CHECK(strstr(result, "{501}{}{A cool tile}\r\n") != nullptr);

    free(result);
}

TEST_CASE("make_PRO_tile_MSG tile_id 0") {
    proto_info info{};
    char name[] = "Zero";
    char desc[] = "Desc";
    info.name = name;
    info.description = desc;

    char* result = make_PRO_tile_MSG(&info, 0);
    REQUIRE(result != nullptr);

    CHECK(strstr(result, "{0}{}{Zero}\r\n") != nullptr);
    CHECK(strstr(result, "{1}{}{Desc}\r\n") != nullptr);

    free(result);
}

TEST_CASE("make_PRO_tile_MSG empty strings") {
    proto_info info{};
    char name[] = "";
    char desc[] = "";
    info.name = name;
    info.description = desc;

    char* result = make_PRO_tile_MSG(&info, 3);
    REQUIRE(result != nullptr);

    CHECK(strstr(result, "{300}{}{}\r\n") != nullptr);
    CHECK(strstr(result, "{301}{}{}\r\n") != nullptr);

    free(result);
}

TEST_CASE("append_PRO_tile_MSG_inplace concat") {
    char* old_msg = (char*)malloc(64);
    strcpy(old_msg, "{100}{}{Name}\r\n");
    char* new_msg = (char*)malloc(64);
    strcpy(new_msg, "{200}{}{Other}\r\n");

    export_state state = {};

    char* result = append_PRO_tile_MSG_inplace(old_msg, new_msg, &state);
    REQUIRE(result != nullptr);
    CHECK(result != old_msg); // new buffer allocated

    CHECK(strstr(result, "{100}{}{Name}\r\n") != nullptr);
    CHECK(strstr(result, "{200}{}{Other}\r\n") != nullptr);

    free(result);
    free(old_msg);
    free(new_msg);
}

TEST_CASE("append_PRO_tile_MSG_inplace null new") {
    char* old_msg = (char*)malloc(64);
    strcpy(old_msg, "{100}{}{Name}\r\n");

    export_state state = {};

    char* result = append_PRO_tile_MSG_inplace(old_msg, nullptr, &state);

    // returns old pointer unchanged
    CHECK(result == old_msg);

    free(old_msg);
}

TEST_CASE("make_PRO_tiles_LST generates entries") {
    tt_arr_handle* h = make_test_handle(3);
    h->tile[0].tile_id = 5;
    h->tile[1].tile_id = 10;
    h->tile[2].tile_id = 15;

    char* result = make_PRO_tiles_LST(h, nullptr);
    REQUIRE(result != nullptr);

    // format: %08d.pro\r\n
    CHECK(strstr(result, "00000005.pro\r\n") != nullptr);
    CHECK(strstr(result, "00000010.pro\r\n") != nullptr);
    CHECK(strstr(result, "00000015.pro\r\n") != nullptr);

    free(result);
    free(h);
}

TEST_CASE("make_PRO_tiles_LST skips tile_id -1") {
    tt_arr_handle* h = make_test_handle(3);
    h->tile[0].tile_id = 5;
    h->tile[1].tile_id = (uint32_t)-1; // blank
    h->tile[2].tile_id = 15;

    char* result = make_PRO_tiles_LST(h, nullptr);
    REQUIRE(result != nullptr);

    CHECK(strstr(result, "00000005.pro\r\n") != nullptr);
    CHECK(strstr(result, "00000015.pro\r\n") != nullptr);
    // only 2 entries * 14 chars each = 28
    CHECK(strlen(result) == 28);

    free(result);
    free(h);
}

TEST_CASE("check_PRO_LST_names null old list") {
    // null tiles_lst -> creates full new list from new_protos
    tt_arr_handle* h = make_test_handle(2);
    h->tile[0].tile_id = 5;
    h->tile[1].tile_id = 10;

    char* result = check_PRO_LST_names(nullptr, h);
    REQUIRE(result != nullptr);

    CHECK(strstr(result, "00000005.pro\r\n") != nullptr);
    CHECK(strstr(result, "00000010.pro\r\n") != nullptr);

    free(result);
    free(h);
}

TEST_CASE("check_PRO_LST_names duplicates excluded") {
    // old list has tile_id 5 (parsed via atoi from "00000005.pro")
    char old_lst[] = "00000005.pro\n";

    tt_arr_handle* h = make_test_handle(2);
    h->tile[0].tile_id = 5;  // duplicate
    h->tile[1].tile_id = 10; // new

    char* result = check_PRO_LST_names(old_lst, h);
    REQUIRE(result != nullptr);

    // 5 is a duplicate, should be excluded
    CHECK(strstr(result, "00000005.pro") == nullptr);
    CHECK(strstr(result, "00000010.pro\r\n") != nullptr);

    free(result);
    free(h);
}

TEST_CASE("append_PRO_tiles_LST appends new") {
    char* old_lst = (char*)malloc(32);
    strcpy(old_lst, "00000005.pro\n");

    tt_arr_handle* h = make_test_handle(1);
    h->tile[0].tile_id = 10; // new entry

    export_state state = {};

    char* result = append_PRO_tiles_LST(old_lst, h, &state);
    REQUIRE(result != nullptr);
    CHECK(result != old_lst);

    CHECK(strstr(result, "00000005.pro\n") != nullptr);
    CHECK(strstr(result, "00000010.pro\r\n") != nullptr);

    free(result);
    free(old_lst);
    free(h);
}

// NOTE: "append_PRO_tiles_LST all duplicates" test omitted because
// when all tiles are duplicates, make_PRO_tiles_LST() calls
// ImGui::OpenPopup() which crashes without an active ImGui context.
