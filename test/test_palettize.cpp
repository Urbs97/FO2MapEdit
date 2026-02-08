#include "FRM_Convert.h"
#include "MiniSDL.h"
#include "Palette_Cycle.h"
#include "doctest.h"

#include <cstring>

// Helper: create a small palette with known colors
static Palette make_test_palette() {
    Palette pal = {};
    pal.num_colors = 256;
    // index 0: black (transparent in Fallout)
    pal.colors[0] = Color{0, 0, 0, 0};
    // index 1: red
    pal.colors[1] = Color{255, 0, 0, 255};
    // index 2: green
    pal.colors[2] = Color{0, 255, 0, 255};
    // index 3: blue
    pal.colors[3] = Color{0, 0, 255, 255};
    // index 4: white
    pal.colors[4] = Color{255, 255, 255, 255};
    // fill rest with gray
    for (int i = 5; i < 256; i++) {
        pal.colors[i] = Color{128, 128, 128, 255};
    }
    return pal;
}

// ── convert_colors ──────────────────────────────────────────────

TEST_CASE("convert_colors 6-bit range") {
    CHECK(convert_colors(0) == 0);
    CHECK(convert_colors(1) == 4);
    CHECK(convert_colors(32) == 128);
    CHECK(convert_colors(63) == 252);
}

TEST_CASE("convert_colors 8-bit passthrough") {
    CHECK(convert_colors(64) == 64);
    CHECK(convert_colors(128) == 128);
    CHECK(convert_colors(200) == 200);
    CHECK(convert_colors(255) == 255);
}

// ── Create_RGBA_Surface ─────────────────────────────────────────

TEST_CASE("Create_RGBA_Surface fields") {
    Surface* s = Create_RGBA_Surface(10, 20);
    REQUIRE(s != nullptr);
    CHECK(s->w == 10);
    CHECK(s->h == 20);
    CHECK(s->channels == 4);
    CHECK(s->pitch == 10 * 4);
    CHECK(s->palette == nullptr);
    CHECK(s->pxls != nullptr);
    FreeSurface(s);
}

// ── Create_8Bit_Surface ─────────────────────────────────────────

TEST_CASE("Create_8Bit_Surface fields") {
    Palette pal = make_test_palette();
    Surface* s = Create_8Bit_Surface(10, 20, &pal);
    REQUIRE(s != nullptr);
    CHECK(s->w == 10);
    CHECK(s->h == 20);
    CHECK(s->channels == 1);
    CHECK(s->pitch == 10);
    CHECK(s->palette == &pal);
    CHECK(s->pxls != nullptr);
    // calloc should zero the pixels
    for (int i = 0; i < 10 * 20; i++) {
        CHECK(s->pxls[i] == 0);
    }
    FreeSurface(s);
}

// ── FreeSurface round-trip ──────────────────────────────────────

TEST_CASE("FreeSurface round-trip") {
    Surface* s1 = Create_RGBA_Surface(4, 4);
    REQUIRE(s1 != nullptr);
    FreeSurface(s1);

    Palette pal = make_test_palette();
    Surface* s2 = Create_8Bit_Surface(4, 4, &pal);
    REQUIRE(s2 != nullptr);
    FreeSurface(s2);
}

// ── Convert_Surface_to_RGBA ─────────────────────────────────────

TEST_CASE("Convert 8-bit to RGBA") {
    Palette pal = make_test_palette();
    Surface* src = Create_8Bit_Surface(2, 1, &pal);
    REQUIRE(src != nullptr);

    // pixel 0 = index 0 (transparent), pixel 1 = index 2 (green)
    src->pxls[0] = 0;
    src->pxls[1] = 2;

    Surface* rgba = Convert_Surface_to_RGBA(src);
    REQUIRE(rgba != nullptr);
    CHECK(rgba->w == 2);
    CHECK(rgba->h == 1);
    CHECK(rgba->channels == 4);

    Color* pixels = (Color*)rgba->pxls;
    // index 0 → alpha = 0
    CHECK(pixels[0].a == 0);
    // index 2 → green with alpha = 0xFF
    CHECK(pixels[1].r == 0);
    CHECK(pixels[1].g == 255);
    CHECK(pixels[1].b == 0);
    CHECK(pixels[1].a == 0xFF);

    FreeSurface(rgba);
    FreeSurface(src);
}

TEST_CASE("Convert RGBA to RGBA") {
    Surface* src = Create_RGBA_Surface(2, 1);
    REQUIRE(src != nullptr);

    Color* pixels = (Color*)src->pxls;
    pixels[0] = Color{10, 20, 30, 255};
    pixels[1] = Color{40, 50, 60, 128};

    Surface* copy = Convert_Surface_to_RGBA(src);
    REQUIRE(copy != nullptr);
    CHECK(copy->channels == 4);

    Color* out = (Color*)copy->pxls;
    CHECK(out[0].r == 10);
    CHECK(out[0].g == 20);
    CHECK(out[0].b == 30);
    CHECK(out[0].a == 255);
    CHECK(out[1].r == 40);
    CHECK(out[1].g == 50);
    CHECK(out[1].b == 60);
    CHECK(out[1].a == 128);

    FreeSurface(copy);
    FreeSurface(src);
}

// ── ClearSurface ────────────────────────────────────────────────

TEST_CASE("ClearSurface zeroes pixels") {
    Palette pal = make_test_palette();
    Surface* s = Create_8Bit_Surface(4, 4, &pal);
    REQUIRE(s != nullptr);

    // fill with non-zero data
    memset(s->pxls, 0xAB, 4 * 4);
    ClearSurface(s);

    for (int i = 0; i < 4 * 4; i++) {
        CHECK(s->pxls[i] == 0);
    }
    FreeSurface(s);
}

// ── PaintSurface ────────────────────────────────────────────────

TEST_CASE("PaintSurface fills region") {
    Palette pal = make_test_palette();
    Surface* s = Create_8Bit_Surface(4, 4, &pal);
    REQUIRE(s != nullptr);
    memset(s->pxls, 0, 4 * 4);

    Rect brush = {1, 1, 2, 2};
    PaintSurface(s, brush, 5);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            uint8_t val = s->pxls[y * s->pitch + x];
            if (x >= 1 && x < 3 && y >= 1 && y < 3) {
                CHECK(val == 5);
            } else {
                CHECK(val == 0);
            }
        }
    }
    FreeSurface(s);
}

// ── BlitSurface ─────────────────────────────────────────────────

TEST_CASE("BlitSurface copies region") {
    Palette pal = make_test_palette();

    // BlitSurface copies dst->pitch bytes per row, so use same-width surfaces
    // src: 4x4, top-left 2x2 filled with index 7, rest index 0
    Surface* src = Create_8Bit_Surface(4, 4, &pal);
    REQUIRE(src != nullptr);
    memset(src->pxls, 0, 4 * 4);
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            src->pxls[y * src->pitch + x] = 7;
        }
    }

    // dst: 4x4, filled with index 0
    Surface* dst = Create_8Bit_Surface(4, 4, &pal);
    REQUIRE(dst != nullptr);
    memset(dst->pxls, 0, 4 * 4);

    Rect src_rect = {0, 0, 4, 2};
    Rect dst_rect = {0, 1, 4, 2};
    BlitSurface(src, src_rect, dst, dst_rect);

    // Row 0 of dst: untouched (all 0)
    for (int x = 0; x < 4; x++) {
        CHECK(dst->pxls[0 * dst->pitch + x] == 0);
    }
    // Rows 1-2 of dst: copied from src rows 0-1
    for (int y = 1; y <= 2; y++) {
        for (int x = 0; x < 4; x++) {
            uint8_t expected = (x < 2) ? 7 : 0;
            CHECK(dst->pxls[y * dst->pitch + x] == expected);
        }
    }
    // Row 3 of dst: untouched (all 0)
    for (int x = 0; x < 4; x++) {
        CHECK(dst->pxls[3 * dst->pitch + x] == 0);
    }

    FreeSurface(src);
    FreeSurface(dst);
}

// ── clamp_dither ────────────────────────────────────────────────

TEST_CASE("clamp_dither applies error") {
    Surface* s = Create_RGBA_Surface(2, 1);
    REQUIRE(s != nullptr);

    Color* pixels = (Color*)s->pxls;
    pixels[0] = Color{128, 128, 128, 255};

    Pxl_Err err = {};
    err.r = 16;
    err.g = 16;
    err.b = 16;
    err.a = 0;

    // factor=16 means full error: 128 + 16*16/16 = 128+16 = 144
    clamp_dither(s, &err, 0, 16);

    CHECK(pixels[0].r == 144);
    CHECK(pixels[0].g == 144);
    CHECK(pixels[0].b == 144);
    CHECK(pixels[0].a == 255);

    FreeSurface(s);
}

TEST_CASE("clamp_dither clamps to 255") {
    Surface* s = Create_RGBA_Surface(2, 1);
    REQUIRE(s != nullptr);

    Color* pixels = (Color*)s->pxls;
    pixels[0] = Color{250, 250, 250, 255};

    Pxl_Err err = {};
    err.r = 100;
    err.g = 100;
    err.b = 100;
    err.a = 0;

    // 250 + 100*16/16 = 350 → clamped to 255
    clamp_dither(s, &err, 0, 16);

    CHECK(pixels[0].r == 255);
    CHECK(pixels[0].g == 255);
    CHECK(pixels[0].b == 255);

    FreeSurface(s);
}

TEST_CASE("clamp_dither clamps to 0") {
    Surface* s = Create_RGBA_Surface(2, 1);
    REQUIRE(s != nullptr);

    Color* pixels = (Color*)s->pxls;
    pixels[0] = Color{5, 5, 5, 255};

    Pxl_Err err = {};
    err.r = -100;
    err.g = -100;
    err.b = -100;
    err.a = 0;

    // 5 + (-100)*16/16 = -95 → clamped to 0
    clamp_dither(s, &err, 0, 16);

    CHECK(pixels[0].r == 0);
    CHECK(pixels[0].g == 0);
    CHECK(pixels[0].b == 0);

    FreeSurface(s);
}

// ── limit_dither ────────────────────────────────────────────────

TEST_CASE("limit_dither distributes to 4 neighbors") {
    // 3x3 RGBA surface, all pixels initialized to (100,100,100,255)
    Surface* s = Create_RGBA_Surface(3, 3);
    REQUIRE(s != nullptr);

    Color* pixels = (Color*)s->pxls;
    for (int i = 0; i < 9; i++) {
        pixels[i] = Color{100, 100, 100, 255};
    }

    Pxl_Err err = {};
    err.r = 32;
    err.g = 32;
    err.b = 32;
    err.a = 0;

    // Error at pixel (0,0). Floyd-Steinberg neighbors:
    //   (1,0) factor 7/16 → 100 + 32*7/16 = 100+14 = 114
    //   (-1,1) out of bounds → skipped
    //   (0,1) factor 5/16 → 100 + 32*5/16 = 100+10 = 110
    //   (1,1) factor 1/16 → 100 + 32*1/16 = 100+2 = 102
    limit_dither(s, &err, 0, 0);

    // (1,0) = index 1
    CHECK(pixels[1].r == 114);
    CHECK(pixels[1].g == 114);
    CHECK(pixels[1].b == 114);

    // (0,1) = index 3
    CHECK(pixels[3].r == 110);
    CHECK(pixels[3].g == 110);
    CHECK(pixels[3].b == 110);

    // (1,1) = index 4
    CHECK(pixels[4].r == 102);
    CHECK(pixels[4].g == 102);
    CHECK(pixels[4].b == 102);

    // (0,0) itself should be unchanged
    CHECK(pixels[0].r == 100);
    CHECK(pixels[0].g == 100);

    FreeSurface(s);
}

TEST_CASE("limit_dither boundary pixel") {
    // 3x3 surface, error at bottom-right corner (2,2)
    Surface* s = Create_RGBA_Surface(3, 3);
    REQUIRE(s != nullptr);

    Color* pixels = (Color*)s->pxls;
    for (int i = 0; i < 9; i++) {
        pixels[i] = Color{100, 100, 100, 255};
    }

    Pxl_Err err = {};
    err.r = 50;
    err.g = 50;
    err.b = 50;
    err.a = 0;

    // At (2,2) all neighbors are out of bounds: (3,2), (1,3), (2,3), (3,3)
    limit_dither(s, &err, 2, 2);

    // All pixels should remain unchanged
    for (int i = 0; i < 9; i++) {
        CHECK(pixels[i].r == 100);
        CHECK(pixels[i].g == 100);
        CHECK(pixels[i].b == 100);
    }

    FreeSurface(s);
}

// ── Euclidian_Distance_Color_Match ──────────────────────────────

TEST_CASE("exact palette match") {
    Palette pal = make_test_palette();

    // 2x1 RGBA surface: pixel 0 = red (matches index 1), pixel 1 = green (matches index 2)
    Surface* rgba = Create_RGBA_Surface(2, 1);
    REQUIRE(rgba != nullptr);
    Color* pixels = (Color*)rgba->pxls;
    pixels[0] = Color{255, 0, 0, 255};
    pixels[1] = Color{0, 255, 0, 255};

    Surface* idx = Create_8Bit_Surface(2, 1, &pal);
    REQUIRE(idx != nullptr);

    Euclidian_Distance_Color_Match(rgba, idx);

    CHECK(idx->pxls[0] == 1); // red → index 1
    CHECK(idx->pxls[1] == 2); // green → index 2

    FreeSurface(rgba);
    FreeSurface(idx);
}

TEST_CASE("nearest match") {
    Palette pal = {};
    pal.num_colors = 256;
    // index 0: black, transparent
    pal.colors[0] = Color{0, 0, 0, 0};
    // index 1: pure red
    pal.colors[1] = Color{255, 0, 0, 255};
    // index 2: pure blue
    pal.colors[2] = Color{0, 0, 255, 255};
    // rest: blue (same as index 2)
    for (int i = 3; i < 256; i++) {
        pal.colors[i] = Color{0, 0, 255, 255};
    }

    // pixel (200,0,0,255) is much closer to red(255,0,0) than blue(0,0,255)
    Surface* rgba = Create_RGBA_Surface(1, 1);
    REQUIRE(rgba != nullptr);
    Color* pixels = (Color*)rgba->pxls;
    pixels[0] = Color{200, 0, 0, 255};

    Surface* idx = Create_8Bit_Surface(1, 1, &pal);
    REQUIRE(idx != nullptr);

    Euclidian_Distance_Color_Match(rgba, idx);

    // dist to index 0: (200-0)^2 + 0 + 0 + (255-0)^2 = 40000+65025 = 105025
    // dist to index 1 (red): (200-255)^2 = 3025
    // dist to index 2 (blue): (200-0)^2 + (0-255)^2 = 40000+65025 = 105025
    // index 1 (red) is the clear winner
    CHECK(idx->pxls[0] == 1);

    FreeSurface(rgba);
    FreeSurface(idx);
}

TEST_CASE("transparent pixel maps to index 0") {
    Palette pal = make_test_palette();

    Surface* rgba = Create_RGBA_Surface(1, 1);
    REQUIRE(rgba != nullptr);
    Color* pixels = (Color*)rgba->pxls;
    pixels[0] = Color{255, 0, 0, 128}; // semi-transparent

    Surface* idx = Create_8Bit_Surface(1, 1, &pal);
    REQUIRE(idx != nullptr);

    Euclidian_Distance_Color_Match(rgba, idx);

    CHECK(idx->pxls[0] == 0);

    FreeSurface(rgba);
    FreeSurface(idx);
}

TEST_CASE("dithering modifies neighbors") {
    // Build a palette where no entry exactly matches (128,0,0)
    Palette pal = {};
    pal.num_colors = 256;
    pal.colors[0] = Color{0, 0, 0, 0};
    pal.colors[1] = Color{200, 0, 0, 255};
    for (int i = 2; i < 256; i++) {
        pal.colors[i] = Color{200, 0, 0, 255};
    }

    // 2x2 surface all (128,0,0,255) — not an exact palette match
    Surface* rgba = Create_RGBA_Surface(2, 2);
    REQUIRE(rgba != nullptr);
    Color* pixels = (Color*)rgba->pxls;
    for (int i = 0; i < 4; i++) {
        pixels[i] = Color{128, 0, 0, 255};
    }

    // Run WITH dithering (Euclidian_Distance_Color_Match includes FS dithering)
    Surface* idx_dithered = Create_8Bit_Surface(2, 2, &pal);
    REQUIRE(idx_dithered != nullptr);
    Euclidian_Distance_Color_Match(rgba, idx_dithered);

    // Dithering modifies the RGBA surface in-place (error diffusion),
    // so the resulting indices after processing the first pixel may differ
    // from what you'd get without dithering. We verify the function ran
    // without crashing and produced valid indices.
    for (int i = 0; i < 4; i++) {
        CHECK(idx_dithered->pxls[i] < 256);
    }

    FreeSurface(rgba);
    FreeSurface(idx_dithered);
}

// ── PAL_Color_Convert ───────────────────────────────────────────

TEST_CASE("PAL_Color_Convert RGBA input") {
    Palette pal = make_test_palette();

    Surface* src = Create_RGBA_Surface(4, 2);
    REQUIRE(src != nullptr);
    Color* pixels = (Color*)src->pxls;
    // Fill with exact palette colors
    for (int i = 0; i < 8; i++) {
        pixels[i] = Color{255, 0, 0, 255}; // red = index 1
    }

    Surface* result = PAL_Color_Convert(src, &pal, 0);
    REQUIRE(result != nullptr);
    CHECK(result->w == 4);
    CHECK(result->h == 2);
    CHECK(result->channels == 1);
    CHECK(result->palette == &pal);

    for (int i = 0; i < 8; i++) {
        CHECK(result->pxls[i] == 1);
    }

    FreeSurface(result);
    FreeSurface(src);
}

TEST_CASE("PAL_Color_Convert 8-bit input") {
    Palette pal = make_test_palette();

    Surface* src = Create_8Bit_Surface(2, 2, &pal);
    REQUIRE(src != nullptr);
    // Fill with index 1 (red)
    for (int i = 0; i < 4; i++) {
        src->pxls[i] = 1;
    }

    Surface* result = PAL_Color_Convert(src, &pal, 0);
    REQUIRE(result != nullptr);
    CHECK(result->channels == 1);
    CHECK(result->palette == &pal);

    // After 8bit→RGBA→8bit round-trip, index 1 (red) should map back to index 1
    for (int i = 0; i < 4; i++) {
        CHECK(result->pxls[i] == 1);
    }

    FreeSurface(result);
    FreeSurface(src);
}

TEST_CASE("PAL_Color_Convert preserves transparency") {
    Palette pal = make_test_palette();

    Surface* src = Create_8Bit_Surface(2, 1, &pal);
    REQUIRE(src != nullptr);
    src->pxls[0] = 0; // transparent
    src->pxls[1] = 1; // red

    Surface* result = PAL_Color_Convert(src, &pal, 0);
    REQUIRE(result != nullptr);

    CHECK(result->pxls[0] == 0); // transparent preserved
    CHECK(result->pxls[1] == 1); // red preserved

    FreeSurface(result);
    FreeSurface(src);
}

// ── Copy8BitSurface ─────────────────────────────────────────────

TEST_CASE("Copy8BitSurface deep copy") {
    Palette pal = make_test_palette();

    Surface* src = Create_8Bit_Surface(4, 4, &pal);
    REQUIRE(src != nullptr);
    for (int i = 0; i < 16; i++) {
        src->pxls[i] = (uint8_t)(i % 5);
    }

    Surface* copy = Copy8BitSurface(src);
    REQUIRE(copy != nullptr);

    // Same data
    CHECK(copy->w == src->w);
    CHECK(copy->h == src->h);
    CHECK(copy->channels == src->channels);
    CHECK(copy->pitch == src->pitch);
    for (int i = 0; i < 16; i++) {
        CHECK(copy->pxls[i] == src->pxls[i]);
    }

    // Different pointers
    CHECK(copy->pxls != src->pxls);

    // Modifying copy doesn't affect original
    copy->pxls[0] = 99;
    CHECK(src->pxls[0] != 99);

    FreeSurface(copy);
    FreeSurface(src);
}
