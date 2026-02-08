#include "B_Endian.h"
#include "doctest.h"

#include <cstring>

TEST_CASE("swap_32 reverses byte order") {
    uint32_t val = 0x01020304;
    B_Endian::swap_32(&val);
    CHECK(val == 0x04030201);
}

TEST_CASE("swap_16 reverses byte order unsigned") {
    uint16_t u = 0x0102;
    B_Endian::swap_16(&u);
    CHECK(u == 0x0201);
}

TEST_CASE("swap_16 reverses byte order signed") {
    int16_t s = 0x0304;
    B_Endian::swap_16(&s);
    CHECK(s == 0x0403);
}

TEST_CASE("flip_header_endian round-trip") {
    FRM_Header original = {};
    original.version = 4;
    original.FPS = 10;
    original.Action_Frame = 0;
    original.Frames_Per_Orient = 1;
    original.Shift_Orient_x[0] = 5;
    original.Shift_Orient_y[2] = -3;
    original.Frame_0_Offset[0] = 0;
    original.Frame_Area = 2892;

    FRM_Header copy = original;

    B_Endian::flip_header_endian(&copy);
    // after one flip, should differ from original (unless all zero)
    CHECK(memcmp(&copy, &original, sizeof(FRM_Header)) != 0);

    B_Endian::flip_header_endian(&copy);
    // after double flip, should match original
    CHECK(copy.version == original.version);
    CHECK(copy.FPS == original.FPS);
    CHECK(copy.Action_Frame == original.Action_Frame);
    CHECK(copy.Frames_Per_Orient == original.Frames_Per_Orient);
    CHECK(copy.Frame_Area == original.Frame_Area);
    for (int i = 0; i < 6; i++) {
        CHECK(copy.Shift_Orient_x[i] == original.Shift_Orient_x[i]);
        CHECK(copy.Shift_Orient_y[i] == original.Shift_Orient_y[i]);
        CHECK(copy.Frame_0_Offset[i] == original.Frame_0_Offset[i]);
    }
}

TEST_CASE("flip_header_endian known value") {
    FRM_Header header = {};
    header.version = 4;
    B_Endian::flip_header_endian(&header);

    // version=4 in big-endian: 0x00000004 -> bytes 00 00 00 04
    uint8_t* bytes = (uint8_t*)&header.version;
    CHECK(bytes[0] == 0x00);
    CHECK(bytes[1] == 0x00);
    CHECK(bytes[2] == 0x00);
    CHECK(bytes[3] == 0x04);
}

TEST_CASE("flip_frame_endian round-trip") {
    FRM_Frame original = {};
    original.Frame_Width = 80;
    original.Frame_Height = 36;
    original.Frame_Size = 80 * 36;
    original.Shift_Offset_x = 10;
    original.Shift_Offset_y = -5;

    FRM_Frame copy = original;

    B_Endian::flip_frame_endian(&copy);
    CHECK(memcmp(&copy, &original, sizeof(FRM_Frame)) != 0);

    B_Endian::flip_frame_endian(&copy);
    CHECK(copy.Frame_Width == original.Frame_Width);
    CHECK(copy.Frame_Height == original.Frame_Height);
    CHECK(copy.Frame_Size == original.Frame_Size);
    CHECK(copy.Shift_Offset_x == original.Shift_Offset_x);
    CHECK(copy.Shift_Offset_y == original.Shift_Offset_y);
}

TEST_CASE("flip_proto_endian round-trip") {
    tile_proto original = {};
    original.ObjectID = 0x04000005;
    original.TextID = 500;
    original.FrmID = 0x04000005;
    original.Light_Radius = 8;
    original.Light_Intensity = 8;
    original.Flags = 0xFFFFFFFF;
    original.MaterialID = 5;

    tile_proto copy = original;

    B_Endian::flip_proto_endian(&copy);
    B_Endian::flip_proto_endian(&copy);

    // These 4 fields ARE swapped, so round-trip should preserve them
    CHECK(copy.ObjectID == original.ObjectID);
    CHECK(copy.TextID == original.TextID);
    CHECK(copy.FrmID == original.FrmID);
    CHECK(copy.MaterialID == original.MaterialID);

    // These 3 fields are NOT swapped (commented out in source),
    // so they should remain unchanged after a single flip too
    CHECK(copy.Light_Radius == original.Light_Radius);
    CHECK(copy.Light_Intensity == original.Light_Intensity);
    CHECK(copy.Flags == original.Flags);
}

TEST_CASE("flip_proto_endian byte layout") {
    tile_proto proto = {};
    proto.ObjectID = 0x04000005;
    B_Endian::flip_proto_endian(&proto);

    // swap_32(0x04000005) -> 0x05000004
    // on little-endian machine, 0x05000004 stored as bytes: 04 00 00 05
    uint8_t* bytes = (uint8_t*)&proto.ObjectID;
    CHECK(bytes[0] == 0x04);
    CHECK(bytes[1] == 0x00);
    CHECK(bytes[2] == 0x00);
    CHECK(bytes[3] == 0x05);
}
