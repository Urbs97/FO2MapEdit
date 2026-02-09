#include "Maps_Txt.h"
#include "doctest.h"

#include <cstdlib>
#include <cstring>

// Helper: duplicate a string literal into a mutable char[] buffer.
// parse_maps_txt_from_buffer() requires a mutable buffer.
static char* make_buffer(const char* src) {
    size_t len = strlen(src);
    char* buf = (char*)malloc(len + 1);
    memcpy(buf, src, len + 1);
    return buf;
}

// Full test data matching test/data/test_maps.txt
static const char* TEST_DATA = "; Test MAPS.TXT - fictional data for unit testing\n"
                               "\n"
                               "[Map 000]\n"
                               "lookup_name=Wasteland Encounter 1\n"
                               "map_name=waste1\n"
                               "music=07theme\n"
                               "ambient_sfx=wind1:30, wind2:30, coyote:20, dust:20\n"
                               "saved=No\n"
                               "dead_bodies_age=No\n"
                               "can_rest_here=No,No,No\n"
                               "random_start_point_0=elev:0, tile_num:10000\n"
                               "random_start_point_1=elev:0, tile_num:10200\n"
                               "\n"
                               "[Map 001]\n"
                               "lookup_name=Hometown Village\n"
                               "map_name=hometown\n"
                               "music=17village\n"
                               "ambient_sfx=birds:25, breeze:25, dogs:20, creek:20, silence:10\n"
                               "saved=Yes\n"
                               "\n"
                               "[Map 002]\n"
                               "lookup_name=Old Mine Shaft\n"
                               "map_name=oldmine\n"
                               "music=13caves\n"
                               "ambient_sfx=drip:30, echo:30, rocks:20, creak:20\n"
                               "saved=Yes\n"
                               "can_rest_here=No,No,No\n"
                               "pipboy_active=No\n"
                               "state=On\n"
                               "\n"
                               "[Map 003]\n"
                               "lookup_name=Minimal Map\n"
                               "map_name=minimal\n"
                               "music=01basic\n"
                               "saved=Yes\n";

TEST_CASE("parse_maps_txt_from_buffer parses map count") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(data->map_count == 4);
    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses map numbers") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    REQUIRE(data->map_count == 4);
    CHECK(data->maps[0].map_number == 0);
    CHECK(data->maps[1].map_number == 1);
    CHECK(data->maps[2].map_number == 2);
    CHECK(data->maps[3].map_number == 3);
    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses lookup_name") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(strcmp(data->maps[0].lookup_name, "Wasteland Encounter 1") == 0);
    CHECK(strcmp(data->maps[1].lookup_name, "Hometown Village") == 0);
    CHECK(strcmp(data->maps[2].lookup_name, "Old Mine Shaft") == 0);
    CHECK(strcmp(data->maps[3].lookup_name, "Minimal Map") == 0);
    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses map_name and music") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(strcmp(data->maps[0].map_name, "waste1") == 0);
    CHECK(strcmp(data->maps[0].music, "07theme") == 0);
    CHECK(strcmp(data->maps[1].map_name, "hometown") == 0);
    CHECK(strcmp(data->maps[1].music, "17village") == 0);
    CHECK(strcmp(data->maps[2].map_name, "oldmine") == 0);
    CHECK(strcmp(data->maps[2].music, "13caves") == 0);
    CHECK(strcmp(data->maps[3].map_name, "minimal") == 0);
    CHECK(strcmp(data->maps[3].music, "01basic") == 0);
    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses ambient_sfx") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);

    // Map 0: 4 entries
    CHECK(data->maps[0].ambient_sfx_count == 4);
    CHECK(strcmp(data->maps[0].ambient_sfx[0].name, "wind1") == 0);
    CHECK(data->maps[0].ambient_sfx[0].weight == 30);
    CHECK(strcmp(data->maps[0].ambient_sfx[1].name, "wind2") == 0);
    CHECK(data->maps[0].ambient_sfx[1].weight == 30);
    CHECK(strcmp(data->maps[0].ambient_sfx[2].name, "coyote") == 0);
    CHECK(data->maps[0].ambient_sfx[2].weight == 20);
    CHECK(strcmp(data->maps[0].ambient_sfx[3].name, "dust") == 0);
    CHECK(data->maps[0].ambient_sfx[3].weight == 20);

    // Map 1: 5 entries
    CHECK(data->maps[1].ambient_sfx_count == 5);
    CHECK(strcmp(data->maps[1].ambient_sfx[0].name, "birds") == 0);
    CHECK(data->maps[1].ambient_sfx[0].weight == 25);
    CHECK(strcmp(data->maps[1].ambient_sfx[4].name, "silence") == 0);
    CHECK(data->maps[1].ambient_sfx[4].weight == 10);

    // Map 3: no ambient_sfx
    CHECK(data->maps[3].ambient_sfx_count == 0);

    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses saved flag") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(data->maps[0].saved == false);
    CHECK(data->maps[1].saved == true);
    CHECK(data->maps[2].saved == true);
    CHECK(data->maps[3].saved == true);
    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses dead_bodies_age") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(data->maps[0].dead_bodies_age == false); // explicit No
    CHECK(data->maps[1].dead_bodies_age == true);  // default
    CHECK(data->maps[2].dead_bodies_age == true);  // default
    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses can_rest_here") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);

    // Map 0: explicit No,No,No
    CHECK(data->maps[0].can_rest_here[0] == false);
    CHECK(data->maps[0].can_rest_here[1] == false);
    CHECK(data->maps[0].can_rest_here[2] == false);

    // Map 1: default (all true)
    CHECK(data->maps[1].can_rest_here[0] == true);
    CHECK(data->maps[1].can_rest_here[1] == true);
    CHECK(data->maps[1].can_rest_here[2] == true);

    // Map 2: explicit No,No,No
    CHECK(data->maps[2].can_rest_here[0] == false);
    CHECK(data->maps[2].can_rest_here[1] == false);
    CHECK(data->maps[2].can_rest_here[2] == false);

    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses random_start_points") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);

    // Map 0: 2 random start points
    CHECK(data->maps[0].random_start_count == 2);
    CHECK(data->maps[0].random_starts[0].elevation == 0);
    CHECK(data->maps[0].random_starts[0].tile_num == 10000);
    CHECK(data->maps[0].random_starts[1].elevation == 0);
    CHECK(data->maps[0].random_starts[1].tile_num == 10200);

    // Map 1: no random starts
    CHECK(data->maps[1].random_start_count == 0);

    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses pipboy_active") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(data->maps[2].pipboy_active == false); // explicit No
    CHECK(data->maps[1].pipboy_active == true);  // default
    CHECK(data->maps[0].pipboy_active == true);  // default
    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer parses state") {
    char* buf = make_buffer(TEST_DATA);
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(data->maps[2].state_on == true);  // explicit On
    CHECK(data->maps[0].state_on == false); // default
    CHECK(data->maps[1].state_on == false); // default
    free(buf);
}

TEST_CASE("parse_maps_txt_from_buffer handles empty input") {
    char buf[] = "";
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(data->map_count == 0);
}

TEST_CASE("parse_maps_txt_from_buffer skips comment-only lines") {
    char buf[] = "; this is a comment\n; another comment\n";
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    CHECK(data->map_count == 0);
}

TEST_CASE("parse_maps_txt_from_buffer inline comments stripped") {
    char buf[] = "[Map 050]\n"
                 "lookup_name=Test Place\n"
                 "map_name=testmap\n"
                 "music=01test\n"
                 "saved=Yes\t; Verified 8/18\n";
    auto data = parse_maps_txt_from_buffer(buf);
    REQUIRE(data != nullptr);
    REQUIRE(data->map_count == 1);
    CHECK(data->maps[0].saved == true);
    CHECK(data->maps[0].map_number == 50);
    CHECK(strcmp(data->maps[0].lookup_name, "Test Place") == 0);
}
