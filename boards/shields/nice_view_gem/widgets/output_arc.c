#include <zephyr/kernel.h>
#include <stdio.h>
#include "output_arc.h"
#include "../assets/custom_fonts.h"

/*
 * Bottom row of the screen:
 *   [BT logo] [profile number]                    WPM 42
 *   [USB logo]                                    WPM 42
 * The Bluetooth logo blinks while the active profile has no connection
 * (searching for / waiting on the host). See state->conn_blink_off.
 */

#define CONN_X      6
#define CONN_Y      143
#define BT_W        10

static const char *const bt_icon[] = {
    "....##....",
    "....###...",
    "....##.#..",
    ".#..##..#.",
    "..#.##.#..",
    "...####...",
    "....##....",
    "...####...",
    "..#.##.#..",
    ".#..##..#.",
    "....##.#..",
    "....###...",
    "....##....",
};

static const char *const usb_icon[] = {
    "..........###.......",
    "..........###.......",
    ".........#..........",
    ".##.....#.........#.",
    "####...#..........##",
    "####################",
    "####......#.......##",
    ".##........#......#.",
    "............#.##....",
    ".............###....",
    "..............#.....",
};

static void draw_icon(lv_obj_t *canvas, int x, int y, const char *const *rows, int n_rows) {
    for (int row = 0; row < n_rows; row++) {
        for (int col = 0; rows[row][col] != '\0'; col++) {
            if (rows[row][col] == '#') {
                lv_canvas_set_px_color(canvas, x + col, y + row, LVGL_FOREGROUND);
            }
        }
    }
}

static void draw_text(lv_obj_t *canvas, int x, int y, int w, const lv_font_t *font,
                      lv_text_align_t align, const char *text) {
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, font, align);
    lv_canvas_draw_text(canvas, x, y, w, &label_dsc, text);
}

void draw_output_status(lv_obj_t *canvas, const struct status_state *state) {
    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        draw_icon(canvas, CONN_X, CONN_Y + 1, usb_icon, ARRAY_SIZE(usb_icon));
        break;
    case ZMK_TRANSPORT_BLE: {
        if (state->active_profile_connected || !state->conn_blink_off) {
            draw_icon(canvas, CONN_X, CONN_Y, bt_icon, ARRAY_SIZE(bt_icon));
        }
        // Profiles are numbered 1..5 on the screen (ZMK counts from 0)
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", state->active_profile_index + 1);
        draw_text(canvas, CONN_X + BT_W + 4, CONN_Y + 1, 20, &quinquefive_12,
                  LV_TEXT_ALIGN_LEFT, buf);
        break;
    }
    default:
        break;
    }

    char wpm[12];
    snprintf(wpm, sizeof(wpm), "WPM %u", state->wpm);
    draw_text(canvas, 0, CONN_Y + 3, SCREEN_WIDTH - 6, &quinquefive_8, LV_TEXT_ALIGN_RIGHT, wpm);
}
