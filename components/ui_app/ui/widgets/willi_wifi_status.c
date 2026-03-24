#include "willi_wifi_status.h"
#include <stdbool.h>

#define C_WIFI 0xEAF3FF

static void set_hidden(lv_obj_t *obj, bool hidden)
{
    if(!obj) return;

    if(hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void wifi_apply_phase(willi_wifi_status_t *w)
{
    lv_opa_t opa = LV_OPA_40;

    if(!w || !w->label) return;

    switch(w->phase) {
        case 0: opa = LV_OPA_30; break;
        case 1: opa = LV_OPA_40; break;
        case 2: opa = LV_OPA_60; break;
        case 3: opa = LV_OPA_90; break;
        case 4: opa = LV_OPA_60; break;
        default: opa = LV_OPA_40; break;
    }

    lv_obj_set_style_text_opa(w->label, opa, 0);
}

static void wifi_apply_strength(willi_wifi_status_t *w, uint8_t level)
{
    if(!w || !w->label) return;

    lv_opa_t opa;
    lv_color_t col;

    switch(level) {
        case 0:
            opa = LV_OPA_30;
            col = lv_color_hex(0x6F7A87);
            break;
        case 1:
            opa = LV_OPA_60;
            col = lv_color_hex(0x9FB3C9);
            break;
        case 2:
            opa = LV_OPA_80;
            col = lv_color_hex(0xBDD7F1);
            break;
        case 3:
            opa = LV_OPA_90;
            col = lv_color_hex(0xD7EEFF);
            break;
        default:
            opa = LV_OPA_COVER;
            col = lv_color_hex(0xEAF3FF);
            break;
    }

    lv_obj_set_style_text_opa(w->label, opa, 0);
    lv_obj_set_style_text_color(w->label, col, 0);
}

static void wifi_timer_cb(lv_timer_t *t)
{
    willi_wifi_status_t *w = (willi_wifi_status_t *)lv_timer_get_user_data(t);
    if(!w) return;

    w->phase = (w->phase + 1) % 6;
    wifi_apply_phase(w);
}

void willi_wifi_status_create(lv_obj_t *parent, willi_wifi_status_t *w, lv_coord_t x, lv_coord_t y)
{
    if(!parent || !w) return;

    w->label = lv_label_create(parent);
    lv_label_set_text(w->label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(w->label, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(w->label, lv_color_hex(C_WIFI), 0);
    lv_obj_set_pos(w->label, x, y);
    lv_obj_clear_flag(w->label, LV_OBJ_FLAG_CLICKABLE);

    w->timer = NULL;
    w->phase = 0;
    wifi_apply_phase(w);
}

void willi_wifi_status_show(willi_wifi_status_t *w)
{
    if(!w) return;
    set_hidden(w->label, false);
}

void willi_wifi_status_hide(willi_wifi_status_t *w)
{
    if(!w) return;
    set_hidden(w->label, true);
}

void willi_wifi_status_start(willi_wifi_status_t *w)
{
    (void)w;
}

void willi_wifi_status_stop(willi_wifi_status_t *w)
{
    (void)w;
}

void willi_wifi_status_set_strength(willi_wifi_status_t *w, uint8_t level)
{
    if (level > 4) level = 4;
    wifi_apply_strength(w, level);
}