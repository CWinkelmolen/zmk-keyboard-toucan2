/*
 * Status screen style 3 for the 144x168 display.
 *
 * Layout inspired by https://github.com/mctechnology17/zmk-dongle-display-view (MIT), rearranged
 * for a portrait screen:
 *
 *   +----------------------------+
 *   | [BT]2v            87% [##] |  output: USB or Bluetooth (+ profile) + status mark
 *   |                   64% [# ] |  batteries: left half, right half
 *   |          Bongo Cat         |
 *   | BASE                       |  layer name
 *   | CAPS                       |  Caps Lock (only when on)
 *   |   ^    opt   cmd   shift   |  modifiers, underlined while held
 *   +----------------------------+
 *
 * The Bluetooth logo blinks while the active profile has no connection.
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/wpm.h>
#include <zmk/split/central.h>
#include <dt-bindings/zmk/modifiers.h>
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>
#endif

#include "screen.h"
#include "bongo_cat.h"
#include "sleep.h"

/* Layout */
#define OUT_Y           2
#define OUT_X           2
#define ICON_W          18      // USB / Bluetooth icon: 18x28

#define BAT_ICON_X      (SCREEN_WIDTH - 12)
#define BAT_ROW_Y(i)    (OUT_Y + (i) * 18)

#define CAT_X           42
#define CAT_Y           40

#define LAYER_X         4
#define LAYER_Y         92
#define CAPS_Y          112

#define MOD_ICON_W      28
#define MOD_GAP         4
#define MOD_Y           134
#define MOD_X(i)        ((SCREEN_WIDTH - (4 * MOD_ICON_W + 3 * MOD_GAP)) / 2 + (i) * (MOD_ICON_W + MOD_GAP))

#define BLINK_INTERVAL_MS 500

LV_IMG_DECLARE(sym_1_2x);
LV_IMG_DECLARE(sym_2_2x);
LV_IMG_DECLARE(sym_3_2x);
LV_IMG_DECLARE(sym_4_2x);
LV_IMG_DECLARE(sym_5_2x);
LV_IMG_DECLARE(sym_ok_2x);
LV_IMG_DECLARE(sym_nok_2x);
LV_IMG_DECLARE(sym_open_2x);
LV_IMG_DECLARE(sym_bt_2x);
LV_IMG_DECLARE(sym_usb_2x);
LV_IMG_DECLARE(control_icon_2x);
LV_IMG_DECLARE(shift_icon_2x);
LV_IMG_DECLARE(alt_icon_2x);
LV_IMG_DECLARE(win_icon_2x);
LV_IMG_DECLARE(cmd_icon_2x);
LV_IMG_DECLARE(opt_icon_2x);

static const lv_img_dsc_t *const profile_digits[] = {
    &sym_1_2x, &sym_2_2x, &sym_3_2x, &sym_4_2x, &sym_5_2x,
};

static const struct {
    uint8_t mods;
    const lv_img_dsc_t *icon;
} modifier_symbols[] = {
#if IS_ENABLED(CONFIG_TOUCAN_MAC_MODIFIERS)
    {MOD_LCTL | MOD_RCTL, &control_icon_2x},
    {MOD_LALT | MOD_RALT, &opt_icon_2x},
    {MOD_LGUI | MOD_RGUI, &cmd_icon_2x},
    {MOD_LSFT | MOD_RSFT, &shift_icon_2x},
#else
    {MOD_LGUI | MOD_RGUI, &win_icon_2x},
    {MOD_LALT | MOD_RALT, &alt_icon_2x},
    {MOD_LCTL | MOD_RCTL, &control_icon_2x},
    {MOD_LSFT | MOD_RSFT, &shift_icon_2x},
#endif
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/**
 * Drawing helpers
 **/

static void draw_img(lv_obj_t *canvas, int x, int y, const lv_img_dsc_t *img) {
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_canvas_draw_img(canvas, x, y, img, &img_dsc);
}

static void draw_box(lv_obj_t *canvas, int x, int y, int w, int h, lv_color_t color) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_width = 0;
    lv_canvas_draw_rect(canvas, x, y, w, h, &dsc);
}

static void draw_label(lv_obj_t *canvas, int x, int y, int w, lv_text_align_t align,
                       const char *text) {
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.color = LVGL_FOREGROUND;
    dsc.font = &lv_font_unscii_16;
    dsc.align = align;
    dsc.letter_space = 1;
    lv_canvas_draw_text(canvas, x, y, w, &dsc, text);
}

static void draw_pattern(lv_obj_t *canvas, int x, int y, const char *const *rows, int n,
                         lv_color_t color) {
    for (int row = 0; row < n; row++) {
        for (int col = 0; rows[row][col] != '\0'; col++) {
            if (rows[row][col] == '#') {
                lv_canvas_set_px_color(canvas, x + col, y + row, color);
            }
        }
    }
}

/**
 * Output: only the active one is shown
 *   USB:       [USB][mark]           mark = ok when the computer is using the keyboard
 *   Bluetooth: [BT][profile][mark]   mark = ok connected / x paired but not connected /
 *                                           <> empty profile, ready to pair
 **/

static void draw_output(lv_obj_t *canvas, const struct status_state *state) {
    if (state->selected_endpoint.transport == ZMK_TRANSPORT_USB) {
        draw_img(canvas, OUT_X, OUT_Y, &sym_usb_2x);
        draw_img(canvas, OUT_X + ICON_W + 2, OUT_Y + 16,
                 state->usb_hid_ready ? &sym_ok_2x : &sym_nok_2x);
        return;
    }

    // Bluetooth, blinking while searching for / waiting on the host
    if (state->active_profile_connected || !state->conn_blink_off) {
        draw_img(canvas, OUT_X, OUT_Y, &sym_bt_2x);
    }
    if (state->active_profile_index >= 0 &&
        state->active_profile_index < ARRAY_SIZE(profile_digits)) {
        draw_img(canvas, OUT_X + ICON_W + 2, OUT_Y, profile_digits[state->active_profile_index]);
    }
    const lv_img_dsc_t *bt_mark = state->active_profile_connected ? &sym_ok_2x
                                  : state->active_profile_bonded  ? &sym_nok_2x
                                                                  : &sym_open_2x;
    draw_img(canvas, OUT_X + ICON_W + 2, OUT_Y + 16, bt_mark);
}

/**
 * Batteries: "87% [icon]" for the left (central) and right (peripheral) half
 **/

static void draw_battery_icon(lv_obj_t *canvas, int x, int y, uint8_t level, bool charging) {
    static const char *const bolt[] = {
        "...##", "..##.", ".##..", "#####", "..##.", ".##..", "##...",
    };

    draw_box(canvas, x + 3, y, 4, 2, LVGL_FOREGROUND);          // terminal
    lv_draw_rect_dsc_t outline;
    lv_draw_rect_dsc_init(&outline);
    outline.bg_opa = LV_OPA_TRANSP;
    outline.border_color = LVGL_FOREGROUND;
    outline.border_width = 1;
    lv_canvas_draw_rect(canvas, x, y + 2, 10, 14, &outline);    // body

    if (charging) {
        draw_pattern(canvas, x + 2, y + 5, bolt, ARRAY_SIZE(bolt), LVGL_FOREGROUND);
        return;
    }

    // Fill from the bottom, 10 steps of 10%
    int rows = (level + 5) / 10;
    if (rows > 10) {
        rows = 10;
    }
    if (rows > 0) {
        draw_box(canvas, x + 2, y + 4 + (10 - rows), 6, rows, LVGL_FOREGROUND);
    }
}

static void draw_battery_row(lv_obj_t *canvas, int row, uint8_t level, bool charging,
                             bool connected) {
    char buf[8];
    if (connected) {
        snprintf(buf, sizeof(buf), "%u%%", level);
    } else {
        snprintf(buf, sizeof(buf), "--");
    }
    int y = BAT_ROW_Y(row);
    draw_label(canvas, 0, y, BAT_ICON_X - 4, LV_TEXT_ALIGN_RIGHT, buf);
    draw_battery_icon(canvas, BAT_ICON_X, y, connected ? level : 0, charging);
}

static void draw_batteries(lv_obj_t *canvas, const struct status_state *state) {
    draw_battery_row(canvas, 0, state->battery, state->charging, true);
    // ZMK reports 0 for the right half while it is disconnected
    draw_battery_row(canvas, 1, state->battery_p, false, state->battery_p > 0);
}

/**
 * Layer name and Caps Lock
 **/

static void draw_layer(lv_obj_t *canvas, const struct status_state *state) {
    const char *name = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(state->layer_index));
    char fallback[8];
    if (name == NULL || name[0] == '\0') {
        snprintf(fallback, sizeof(fallback), "L%u", state->layer_index);
        name = fallback;
    }
    draw_label(canvas, LAYER_X, LAYER_Y, SCREEN_WIDTH - LAYER_X, LV_TEXT_ALIGN_LEFT, name);

    if (state->caps_lock) {
        draw_label(canvas, LAYER_X, CAPS_Y, SCREEN_WIDTH - LAYER_X, LV_TEXT_ALIGN_LEFT, "CAPS");
    }
}

/**
 * Modifiers: active ones move up 2 px and get a line underneath
 **/

static void draw_modifiers(lv_obj_t *canvas, const struct status_state *state) {
    for (int i = 0; i < ARRAY_SIZE(modifier_symbols); i++) {
        bool active = (state->mods & modifier_symbols[i].mods) != 0;
        int x = MOD_X(i);
        draw_img(canvas, x, active ? MOD_Y - 2 : MOD_Y, modifier_symbols[i].icon);
        if (active) {
            draw_box(canvas, x + 2, MOD_Y + MOD_ICON_W - 2, MOD_ICON_W - 4, 2, LVGL_FOREGROUND);
        }
    }
}

static void draw_screen(struct zmk_widget_screen *widget) {
    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    fill_background(canvas);

    if (is_sleep_screen_active()) {
        draw_sleep_screen(canvas);
        return;
    }

    draw_output(canvas, &widget->state);
    draw_batteries(canvas, &widget->state);
    draw_layer(canvas, &widget->state);
    draw_modifiers(canvas, &widget->state);
}

#define FOR_EACH_WIDGET(widget) SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node)

/**
 * Battery (left half)
 **/

struct battery_state {
    uint8_t level;
    bool usb_present;
};

static void battery_update_cb(struct battery_state state) {
    struct zmk_widget_screen *widget;
    FOR_EACH_WIDGET(widget) {
        widget->state.battery = state.level;
        widget->state.charging = state.usb_present;
        draw_screen(widget);
    }
}

static struct battery_state battery_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery, struct battery_state, battery_update_cb,
                            battery_get_state)
ZMK_SUBSCRIPTION(widget_battery, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery, zmk_usb_conn_state_changed);
#endif

/**
 * Battery (right half)
 **/

struct peripheral_battery_state {
    uint8_t level;
};

static void peripheral_battery_update_cb(struct peripheral_battery_state state) {
    struct zmk_widget_screen *widget;
    FOR_EACH_WIDGET(widget) {
        widget->state.battery_p = state.level;
        draw_screen(widget);
    }
}

static struct peripheral_battery_state peripheral_battery_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    uint8_t level = 0;
    if (ev != NULL) {
        level = ev->state_of_charge;
    } else {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
        zmk_split_central_get_peripheral_battery_level(0, &level);
#endif
    }
    return (struct peripheral_battery_state){.level = level};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_battery, struct peripheral_battery_state,
                            peripheral_battery_update_cb, peripheral_battery_get_state)
ZMK_SUBSCRIPTION(widget_peripheral_battery, zmk_peripheral_battery_state_changed);

/**
 * Output (USB / Bluetooth)
 **/

struct output_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    bool usb_hid_ready;
};

static void output_update_cb(struct output_state state) {
    struct zmk_widget_screen *widget;
    FOR_EACH_WIDGET(widget) {
        widget->state.selected_endpoint = state.selected_endpoint;
        widget->state.active_profile_index = state.active_profile_index;
        widget->state.active_profile_connected = state.active_profile_connected;
        widget->state.active_profile_bonded = state.active_profile_bonded;
        widget->state.usb_hid_ready = state.usb_hid_ready;

        // Only blink while Bluetooth is the active output and has no connection
        if (state.selected_endpoint.transport != ZMK_TRANSPORT_BLE ||
            state.active_profile_connected) {
            lv_timer_pause(widget->blink_timer);
            widget->state.conn_blink_off = false;
        } else {
            lv_timer_resume(widget->blink_timer);
        }
        draw_screen(widget);
    }
}

static struct output_state output_get_state(const zmk_event_t *eh) {
    return (struct output_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
#if IS_ENABLED(CONFIG_ZMK_USB)
        .usb_hid_ready = zmk_usb_is_hid_ready(),
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output, struct output_state, output_update_cb,
                            output_get_state)
ZMK_SUBSCRIPTION(widget_output, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output, zmk_usb_conn_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output, zmk_ble_active_profile_changed);
#endif

static void blink_timer_cb(lv_timer_t *timer) {
    struct zmk_widget_screen *widget = timer->user_data;
    widget->state.conn_blink_off = !widget->state.conn_blink_off;
    draw_screen(widget);
}

/**
 * Layer
 **/

struct layer_state {
    uint8_t index;
};

static void layer_update_cb(struct layer_state state) {
    struct zmk_widget_screen *widget;
    FOR_EACH_WIDGET(widget) {
        widget->state.layer_index = state.index;
        draw_screen(widget);
    }
}

static struct layer_state layer_get_state(const zmk_event_t *eh) {
    return (struct layer_state){.index = zmk_keymap_highest_layer_active()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer, struct layer_state, layer_update_cb, layer_get_state)
ZMK_SUBSCRIPTION(widget_layer, zmk_layer_state_changed);

/**
 * Modifiers
 *
 * ZMK v0.3 does not raise modifiers_state_changed, so listen to keycode events and read the
 * explicit modifiers again in the display work callback, after the event has been handled.
 **/

struct mods_state {
    uint8_t mods;
};

static void mods_update_cb(struct mods_state state) {
    uint8_t mods = zmk_hid_get_explicit_mods();
    struct zmk_widget_screen *widget;
    FOR_EACH_WIDGET(widget) {
        // keycode events fire on every key press, only redraw on a change
        if (widget->state.mods != mods) {
            widget->state.mods = mods;
            draw_screen(widget);
        }
    }
}

static struct mods_state mods_get_state(const zmk_event_t *eh) {
    return (struct mods_state){.mods = zmk_hid_get_explicit_mods()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_mods, struct mods_state, mods_update_cb, mods_get_state)
ZMK_SUBSCRIPTION(widget_mods, zmk_keycode_state_changed);

/**
 * Caps Lock (LED state sent by the host)
 **/

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#define HID_INDICATOR_CAPS_LOCK BIT(1)

struct caps_state {
    bool caps_lock;
};

static void caps_update_cb(struct caps_state state) {
    struct zmk_widget_screen *widget;
    FOR_EACH_WIDGET(widget) {
        widget->state.caps_lock = state.caps_lock;
        draw_screen(widget);
    }
}

static struct caps_state caps_get_state(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);
    zmk_hid_indicators_t indicators =
        (ev != NULL) ? ev->indicators : zmk_hid_indicators_get_current_profile();
    return (struct caps_state){.caps_lock = (indicators & HID_INDICATOR_CAPS_LOCK) != 0};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_caps, struct caps_state, caps_update_cb, caps_get_state)
ZMK_SUBSCRIPTION(widget_caps, zmk_hid_indicators_changed);
#endif

/**
 * WPM -> Bongo Cat speed
 **/

struct wpm_state {
    uint8_t wpm;
};

static void wpm_update_cb(struct wpm_state state) {
    struct zmk_widget_screen *widget;
    FOR_EACH_WIDGET(widget) {
        widget->state.wpm = state.wpm;
        bongo_cat_set_wpm(widget->cat, state.wpm);
    }
}

static struct wpm_state wpm_get_state(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    return (struct wpm_state){.wpm = (ev != NULL) ? ev->state : zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm, struct wpm_state, wpm_update_cb, wpm_get_state)
ZMK_SUBSCRIPTION(widget_wpm, zmk_wpm_state_changed);

/**
 * Sleep screen
 **/

static int activity_event_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    struct zmk_widget_screen *widget;
    switch (ev->state) {
    case ZMK_ACTIVITY_ACTIVE:
        set_sleep_screen_active(false);
        FOR_EACH_WIDGET(widget) { bongo_cat_set_hidden(widget->cat, false); }
        break;
    case ZMK_ACTIVITY_SLEEP:
        set_sleep_screen_active(true);
        FOR_EACH_WIDGET(widget) {
            bongo_cat_set_hidden(widget->cat, true);
            lv_timer_pause(widget->blink_timer);
            draw_screen(widget);
        }
        // Flush to the display before the CPU enters deep sleep
        lv_task_handler();
        lv_refr_now(NULL);
        break;
    default:
        break;
    }
    return 0;
}

ZMK_LISTENER(nice_view_gem_display, activity_event_handler);
ZMK_SUBSCRIPTION(nice_view_gem_display, zmk_activity_state_changed);

/**
 * Initialization
 **/

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, SCREEN_WIDTH, SCREEN_HEIGHT);

    lv_obj_t *canvas = lv_canvas_create(widget->obj);
    lv_obj_align(canvas, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(canvas, widget->cbuf, SCREEN_WIDTH, SCREEN_HEIGHT, LV_IMG_CF_TRUE_COLOR);

    // Created after the canvas so it is drawn on top of it
    widget->cat = bongo_cat_create(widget->obj, CAT_X, CAT_Y);
    widget->blink_timer = lv_timer_create(blink_timer_cb, BLINK_INTERVAL_MS, widget);
    lv_timer_pause(widget->blink_timer);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_init();
    widget_peripheral_battery_init();
    widget_output_init();
    widget_layer_init();
    widget_mods_init();
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    widget_caps_init();
#endif
    widget_wpm_init();

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) { return widget->obj; }
