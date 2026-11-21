#pragma once

#include <lvgl.h>

#define BONGO_CAT_X 22
#define BONGO_CAT_Y 58

/* Creates the animated Bongo Cat image object on top of the status canvas */
lv_obj_t *bongo_cat_create(lv_obj_t *parent);

/* Picks the animation (idle / slow / mid / fast) that matches the current WPM */
void bongo_cat_set_wpm(lv_obj_t *cat, uint8_t wpm);

/* Hides the cat (e.g. while the sleep screen is shown) */
void bongo_cat_set_hidden(lv_obj_t *cat, bool hidden);
