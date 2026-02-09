#include "B_Endian.h"
#include "Proto_Files.h"
#include "doctest.h"
#include "town_map_tiles.h"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

TEST_CASE("save_TMAP_tile_FRM valid binary") {
    // create temp file path
    char tmp_path[] = "/tmp/test_frm_XXXXXX";
    int fd = mkstemp(tmp_path);
    REQUIRE(fd != -1);
    close(fd);

    uint8_t pxls[80 * 36];
    memset(pxls, 42, sizeof(pxls));

    char name[] = "test.FRM";
    save_TMAP_tile_FRM(tmp_path, pxls, name);

    // read back the file
    FILE* f = fopen(tmp_path, "rb");
    REQUIRE(f != nullptr);

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // expected: sizeof(FRM_Header) + sizeof(FRM_Frame w/o flex array) + 80*36
    // = 62 + 12 + 2880 = 2954
    CHECK(file_size == 2954);

    // read header
    FRM_Header header = {};
    fread(&header, sizeof(FRM_Header), 1, f);
    // header is big-endian on disk, flip back to native
    B_Endian::flip_header_endian(&header);
    CHECK(header.version == 4);
    CHECK(header.FPS == 1);
    CHECK(header.Frames_Per_Orient == 1);
    CHECK(header.Frame_Area == (uint32_t)(80 * 36 + sizeof(FRM_Frame)));

    // read frame
    FRM_Frame frame = {};
    fread(&frame, 12, 1, f); // 12 bytes (no flexible array member)
    B_Endian::flip_frame_endian(&frame);
    CHECK(frame.Frame_Width == 80);
    CHECK(frame.Frame_Height == 36);
    CHECK(frame.Frame_Size == (uint32_t)(80 * 36));

    // read pixel data
    uint8_t read_pxls[80 * 36];
    fread(read_pxls, 80 * 36, 1, f);
    CHECK(memcmp(read_pxls, pxls, 80 * 36) == 0);

    fclose(f);
    remove(tmp_path);
}

TEST_CASE("export_single_tile_PRO valid binary") {
    // create temp directory structure
    char tmp_dir[] = "/tmp/test_pro_XXXXXX";
    REQUIRE(mkdtemp(tmp_dir) != nullptr);

    // export_single_tile_PRO builds path:
    //   game_path/data/proto/tiles/%08d.pro
    // We need to create those subdirectories
    char sub_path[256];
    snprintf(sub_path, sizeof(sub_path), "%s/data", tmp_dir);
    mkdir(sub_path, 0755);
    snprintf(sub_path, sizeof(sub_path), "%s/data/proto", tmp_dir);
    mkdir(sub_path, 0755);
    snprintf(sub_path, sizeof(sub_path), "%s/data/proto/tiles", tmp_dir);
    mkdir(sub_path, 0755);

    tt_arr tile = {};
    tile.tile_id = 5;
    strncpy(tile.name_ptr, "tile_005.FRM", 13);

    proto_info info{};
    char name[] = "TestTile";
    char desc[] = "A test";
    info.name = name;
    info.description = desc;
    info.material_id = 5; // Stone
    info.pro_tile = 500;

    bool success = export_single_tile_PRO(tmp_dir, &tile, &info);
    CHECK(success == true);

    // verify the file
    char pro_path[256];
    snprintf(pro_path, sizeof(pro_path), "%s/data/proto/tiles/00000005.pro", tmp_dir);

    FILE* f = fopen(pro_path, "rb");
    REQUIRE(f != nullptr);

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // tile_proto = 7 x uint32_t = 28 bytes
    CHECK(file_size == 28);

    tile_proto proto = {};
    fread(&proto, sizeof(tile_proto), 1, f);
    fclose(f);

    // flip back from big-endian
    B_Endian::flip_proto_endian(&proto);

    // ObjectID = tile_id | 0x4000000
    CHECK(proto.ObjectID == (uint32_t)(5 | 0x4000000));
    // FrmID = tile_id | 0x4000000
    CHECK(proto.FrmID == (uint32_t)(5 | 0x4000000));
    // TextID = info.pro_tile
    CHECK(proto.TextID == 500);
    // MaterialID = info.material_id
    CHECK(proto.MaterialID == 5);

    // clean up
    remove(pro_path);
    snprintf(sub_path, sizeof(sub_path), "%s/data/proto/tiles", tmp_dir);
    rmdir(sub_path);
    snprintf(sub_path, sizeof(sub_path), "%s/data/proto", tmp_dir);
    rmdir(sub_path);
    snprintf(sub_path, sizeof(sub_path), "%s/data", tmp_dir);
    rmdir(sub_path);
    rmdir(tmp_dir);
}
