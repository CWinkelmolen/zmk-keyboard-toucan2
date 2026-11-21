/*
 * Bongo Cat that drums along with your typing speed.
 * Animation timings follow https://github.com/mctechnology17/zmk-dongle-display-view (MIT).
 */
#include <zephyr/kernel.h>
#include "bongo_cat.h"

LV_IMG_DECLARE(bongo_cat_none);
LV_IMG_DECLARE(bongo_cat_left1);
LV_IMG_DECLARE(bongo_cat_left2);
LV_IMG_DECLARE(bongo_cat_right1);
LV_IMG_DECLARE(bongo_cat_right2);
LV_IMG_DECLARE(bongo_cat_both1);
LV_IMG_DECLARE(bongo_cat_both1_open);
LV_IMG_DECLARE(bongo_cat_both2);

#define SRC(array) (const void **)array, ARRAY_SIZE(array)

/* Not typing: blinks now and then */
#define ANIMATION_SPEED_IDLE 10000
static const lv_img_dsc_t *idle_imgs[] = {
    &bongo_cat_both1_open,
    &bongo_cat_both1_open,
    &bongo_cat_both1_open,
    &bongo_cat_both1,
};

#define ANIMATION_SPEED_SLOW 2000
static const lv_img_dsc_t *slow_imgs[] = {
    &bongo_cat_left1, &bongo_cat_both1, &bongo_cat_both1,
    &bongo_cat_right1, &bongo_cat_both1, &bongo_cat_both1,
    &bongo_cat_left1, &bongo_cat_both1, &bongo_cat_both1,
};

#define ANIMATION_SPEED_MID 500
static const lv_img_dsc_t *mid_imgs[] = {
    &bongo_cat_left2, &bongo_cat_left1, &bongo_cat_none,
    &bongo_cat_right2, &bongo_cat_right1, &bongo_cat_none,
};

#define ANIMATION_SPEED_FAST 200
static const lv_img_dsc_t *fast_imgs[] = {
    &bongo_cat_both2,
    &bongo_cat_both1,
    &bongo_cat_none,
    &bongo_cat_none,
};

enum anim_state {
    ANIM_NONE,
    ANIM_IDLE,
    ANIM_SLOW,
    ANIM_MID,
    ANIM_FAST,
};

static enum anim_state current_anim = ANIM_NONE;

static void start_animation(lv_obj_t *cat, const lv_img_dsc_t **imgs, uint8_t count,
                            uint32_t duration) {
    lv_animimg_set_src(cat, (const void **)imgs, count);
    lv_animimg_set_duration(cat, duration);
    lv_animimg_set_repeat_count(cat, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(cat);
}

void bongo_cat_set_wpm(lv_obj_t *cat, uint8_t wpm) {
    enum anim_state wanted;
    if (wpm < 5) {
        wanted = ANIM_IDLE;
    } else if (wpm < 30) {
        wanted = ANIM_SLOW;
    } else if (wpm < 70) {
        wanted = ANIM_MID;
    } else {
        wanted = ANIM_FAST;
    }

    if (wanted == current_anim) {
        return;
    }
    current_anim = wanted;

    switch (wanted) {
    case ANIM_IDLE:
        start_animation(cat, idle_imgs, ARRAY_SIZE(idle_imgs), ANIMATION_SPEED_IDLE);
        break;
    case ANIM_SLOW:
        start_animation(cat, slow_imgs, ARRAY_SIZE(slow_imgs), ANIMATION_SPEED_SLOW);
        break;
    case ANIM_MID:
        start_animation(cat, mid_imgs, ARRAY_SIZE(mid_imgs), ANIMATION_SPEED_MID);
        break;
    default:
        start_animation(cat, fast_imgs, ARRAY_SIZE(fast_imgs), ANIMATION_SPEED_FAST);
        break;
    }
}

void bongo_cat_set_hidden(lv_obj_t *cat, bool hidden) {
    if (hidden) {
        lv_obj_add_flag(cat, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(cat, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *bongo_cat_create(lv_obj_t *parent, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *cat = lv_animimg_create(parent);
    lv_obj_set_pos(cat, x, y);
    lv_img_set_src(cat, &bongo_cat_both1_open);
    current_anim = ANIM_NONE;
    bongo_cat_set_wpm(cat, 0);
    return cat;
}
