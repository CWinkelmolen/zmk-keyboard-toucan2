#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "util.h"

struct zmk_widget_screen {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_color_t cbuf[SCREEN_WIDTH * SCREEN_HEIGHT];
    lv_color_t cbuf2[SCREEN_WIDTH * SCREEN_HEIGHT];
    lv_color_t cbuf3[SCREEN_WIDTH * SCREEN_HEIGHT];
    struct status_state state;
#if defined(CONFIG_TOUCAN_STATUS_SCREEN) && CONFIG_TOUCAN_STATUS_SCREEN == 2
    lv_obj_t *cat;              // animated Bongo Cat, drawn on top of the canvas
    lv_timer_t *blink_timer;    // blinks the Bluetooth logo while not connected
#endif
};

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget);
