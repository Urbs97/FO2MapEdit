#include "dat2/dat2_io.h"
#include "doctest.h"

#include <cstdint>

TEST_CASE("dat2::read_le_u32 basic") {
    // 0x04030201 in little-endian bytes: 01 02 03 04
    uint8_t buf[] = { 0x01, 0x02, 0x03, 0x04 };
    uint32_t val = 0;
    REQUIRE(dat2::read_le_u32(buf, sizeof(buf), 0, val));
    CHECK(val == 0x04030201);
}

TEST_CASE("dat2::read_le_u32 offset") {
    uint8_t buf[] = { 0xFF, 0x01, 0x02, 0x03, 0x04 };
    uint32_t val = 0;
    REQUIRE(dat2::read_le_u32(buf, sizeof(buf), 1, val));
    CHECK(val == 0x04030201);
}

TEST_CASE("dat2::read_le_u32 zero value") {
    uint8_t buf[] = { 0x00, 0x00, 0x00, 0x00 };
    uint32_t val = 42;
    REQUIRE(dat2::read_le_u32(buf, sizeof(buf), 0, val));
    CHECK(val == 0);
}

TEST_CASE("dat2::read_le_u32 max value") {
    uint8_t buf[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    uint32_t val = 0;
    REQUIRE(dat2::read_le_u32(buf, sizeof(buf), 0, val));
    CHECK(val == 0xFFFFFFFF);
}

TEST_CASE("dat2::read_le_u32 out of bounds") {
    uint8_t buf[] = { 0x01, 0x02, 0x03 };
    uint32_t val = 42;
    CHECK_FALSE(dat2::read_le_u32(buf, sizeof(buf), 0, val));
    // val should be unchanged on failure
    CHECK(val == 42);
}

TEST_CASE("dat2::read_le_u32 out of bounds at edge") {
    uint8_t buf[] = { 0x01, 0x02, 0x03, 0x04 };
    uint32_t val = 42;
    // offset 1 means we need bytes 1..4, but index 4 is past the end
    CHECK_FALSE(dat2::read_le_u32(buf, sizeof(buf), 1, val));
    CHECK(val == 42);
}

TEST_CASE("dat2::read_le_u32 empty buffer") {
    uint32_t val = 42;
    CHECK_FALSE(dat2::read_le_u32(nullptr, 0, 0, val));
    CHECK(val == 42);
}

TEST_CASE("dat2::read_le_u8 basic") {
    uint8_t buf[] = { 0xAB };
    uint8_t val = 0;
    REQUIRE(dat2::read_le_u8(buf, sizeof(buf), 0, val));
    CHECK(val == 0xAB);
}

TEST_CASE("dat2::read_le_u8 offset") {
    uint8_t buf[] = { 0x00, 0x42 };
    uint8_t val = 0;
    REQUIRE(dat2::read_le_u8(buf, sizeof(buf), 1, val));
    CHECK(val == 0x42);
}

TEST_CASE("dat2::read_le_u8 out of bounds") {
    uint8_t buf[] = { 0xAB };
    uint8_t val = 42;
    CHECK_FALSE(dat2::read_le_u8(buf, sizeof(buf), 1, val));
    CHECK(val == 42);
}

TEST_CASE("dat2::read_le_u8 empty buffer") {
    uint8_t val = 42;
    CHECK_FALSE(dat2::read_le_u8(nullptr, 0, 0, val));
    CHECK(val == 42);
}
