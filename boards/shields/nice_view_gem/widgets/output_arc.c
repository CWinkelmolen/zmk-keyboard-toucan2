#include <zephyr/kernel.h>
#include "output_arc.h"
#include "../assets/custom_fonts.h"

static void draw_output_text(lv_obj_t *canvas, const char *text) {
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &quinquefive_8, LV_TEXT_ALIGN_RIGHT);
    lv_canvas_draw_text(canvas, -18, 143, SCREEN_WIDTH-8, &label_dsc, text);
}

/*
 * USB  - output goes to USB
 * BLE  - active Bluetooth profile is connected
 * WAIT - active profile is paired, but the host is not connected (yet)
 * PAIR - active profile is empty and ready to pair with a new host
 */
void draw_output_status(lv_obj_t *canvas, const struct status_state *state) {
    switch (state->selected_endpoint.transport) {
        case ZMK_TRANSPORT_USB:
            draw_output_text(canvas, "USB");
            break;
        case ZMK_TRANSPORT_BLE:
            if (state->active_profile_connected) {
                draw_output_text(canvas, "BLE");
            } else if (state->active_profile_bonded) {
                draw_output_text(canvas, "WAIT");
            } else {
                draw_output_text(canvas, "PAIR");
            }
            break;
        default:
            draw_output_text(canvas, "NULL");
            break;
    }
}
