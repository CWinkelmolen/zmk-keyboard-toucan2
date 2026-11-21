#pragma once

#include <lvgl.h>
#include "util.h"

struct mods_status_state {
    uint8_t mods;
};

struct caps_status_state {
    bool caps_lock;
};

void draw_wpm_value(lv_obj_t *canvas, const struct status_state *state);
void draw_mods_status(lv_obj_t *canvas, const struct status_state *state);
void draw_caps_status(lv_obj_t *canvas, const struct status_state *state);
