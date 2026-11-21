#include <zephyr/kernel.h>
#include <stdio.h>

#include "indicators.h"
#include "../assets/custom_fonts.h"

/*
 * Extra status indicators for status screen style 2:
 *  - Modifier boxes (S C A G) in the top-left corner; active ones are drawn inverted
 *  - CAPS badge in the top-right corner (only when Caps Lock is on)
 *  - WPM value above the WPM chart
 *
 * quinquefive_8 glyphs are 8x9 px with a 10 px advance; the boxes leave 2 px around them.
 */

#define TOP_Y           1       // strip above the battery arcs
#define BOX_H           13
#define TEXT_PAD        2

#define MOD_BOX_W       12
#define MOD_BOX_GAP     2
#define MOD_COUNT       4
#define MOD_X           2

#define CAPS_W          42
#define CAPS_X          (SCREEN_WIDTH - 2 - CAPS_W)

#define WPM_X           12
#define WPM_Y           62      // row between the battery arcs and the WPM chart

/* Bit order in state->mods: CTRL, SHIFT, ALT, GUI. Display order: S C A G. */
static const struct {
    uint8_t bit;
    const char *label;
} mod_slots[MOD_COUNT] = {
    {BIT(1), "S"},
    {BIT(0), "C"},
    {BIT(2), "A"},
    {BIT(3), "G"},
};

static void draw_box(lv_obj_t *canvas, int x, int y, int w, int h, lv_color_t color) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_width = 0;
    dsc.radius = 2;
    lv_canvas_draw_rect(canvas, x, y, w, h, &dsc);
}

static void draw_small_text(lv_obj_t *canvas, int x, int y, int w, lv_color_t color,
                            const char *text) {
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, color, &quinquefive_8, LV_TEXT_ALIGN_LEFT);
    lv_canvas_draw_text(canvas, x, y, w, &label_dsc, text);
}

void draw_wpm_value(lv_obj_t *canvas, const struct status_state *state) {
    char buf[12];
    snprintf(buf, sizeof(buf), "WPM %u", state->wpm);
    draw_small_text(canvas, WPM_X, WPM_Y, 80, LVGL_FOREGROUND, buf);
}

void draw_mods_status(lv_obj_t *canvas, const struct status_state *state) {
    for (int i = 0; i < MOD_COUNT; i++) {
        int x = MOD_X + i * (MOD_BOX_W + MOD_BOX_GAP);
        bool active = (state->mods & mod_slots[i].bit) != 0;

        if (active) {
            draw_box(canvas, x, TOP_Y, MOD_BOX_W, BOX_H, LVGL_FOREGROUND);
        }
        draw_small_text(canvas, x + TEXT_PAD, TOP_Y + 1, MOD_BOX_W,
                        active ? LVGL_BACKGROUND : LVGL_FOREGROUND, mod_slots[i].label);
    }
}

void draw_caps_status(lv_obj_t *canvas, const struct status_state *state) {
    if (!state->caps_lock) {
        return;
    }

    draw_box(canvas, CAPS_X, TOP_Y, CAPS_W, BOX_H, LVGL_FOREGROUND);
    draw_small_text(canvas, CAPS_X + TEXT_PAD, TOP_Y + 1, CAPS_W, LVGL_BACKGROUND, "CAPS");
}
