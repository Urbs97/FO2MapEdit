#include "doctest.h"
#include "timer_functions.h"
#include "town_map_tiles.h"

#include <cstring>

#define TILE_W 80
#define TILE_H 36

TEST_CASE("crop_single_tile stays in bounds") {
    int frm_w = 100;
    int frm_h = 40;
    uint8_t tile_buff[TILE_W * TILE_H * 3];
    uint8_t frm_pxls[100 * 40];

    // sentinel value in entire buffer
    memset(tile_buff, 168, TILE_W * TILE_H * 3);
    // frm pixels filled with a visible color
    memset(frm_pxls, 133, frm_w * frm_h);

    uint64_t start_time = start_timer();

    for (int y = -36; y < frm_h; y++) {
        for (int x = -80; x < frm_w; x++) {
            // reset middle tile region
            memset(tile_buff + TILE_W * TILE_H, 216, TILE_W * TILE_H);
            uint8_t* buff_ptr = tile_buff + TILE_W * TILE_H;

            crop_single_tile(buff_ptr, frm_pxls, frm_w, frm_h, x, y);

            // check guard region before tile
            bool before_ok = true;
            for (int i = 0; i < TILE_W * TILE_H; i++) {
                if (tile_buff[i] != 168) {
                    before_ok = false;
                    break;
                }
            }
            CHECK_MESSAGE(before_ok, "guard region BEFORE tile overwritten at x=", x, " y=", y);

            // check guard region after tile
            bool after_ok = true;
            uint8_t* after_ptr = tile_buff + 2 * TILE_W * TILE_H;
            for (int i = 0; i < TILE_W * TILE_H; i++) {
                if (after_ptr[i] != 168) {
                    after_ok = false;
                    break;
                }
            }
            CHECK_MESSAGE(after_ok, "guard region AFTER tile overwritten at x=", x, " y=", y);

            if (!before_ok || !after_ok) {
                // stop early on first failure to avoid flooding output
                print_timer(start_time);
                return;
            }
        }
    }

    print_timer(start_time);
}
