#include "willi_stop_btn.h"
#include <stdlib.h>

#define STOP_BG_1       0x271316
#define STOP_BG_2       0x3A171B
#define STOP_RED        0xFF4A4A
#define STOP_RED_SOFT   0xFF7C7C
#define STOP_BORDER     0x7C4A4A
#define STOP_TEXT       0xFFF4F4

static void style_root(lv_obj_t *obj)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 240, 64);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_radius(obj, 14, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(STOP_BG_1), 0);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(STOP_BG_2), 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(STOP_BORDER), 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_50, 0);

    lv_obj_set_style_shadow_color(obj, lv_color_hex(STOP_RED), 0);
    lv_obj_set_style_shadow_width(obj, 12, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_20, 0);

    lv_obj_set_style_pad_all(obj, 0, 0);
}

static void style_face(lv_obj_t *obj)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_center(obj);

    lv_obj_set_style_radius(obj, 14, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_set_style_transform_zoom(obj, 256, 0);
    lv_obj_set_style_transform_pivot_x(obj, 115, 0);
    lv_obj_set_style_transform_pivot_y(obj, 30, 0);
}

static void press_y_cb(void *var, int32_t v)
{
    lv_obj_set_style_translate_y((lv_obj_t *)var, (lv_coord_t)v, 0);
}

static void press_zoom_cb(void *var, int32_t v)
{
    lv_obj_set_style_transform_zoom((lv_obj_t *)var, (lv_coord_t)v, 0);
}

static void event_cb(lv_event_t *e)
{
    willi_stop_btn_t *btn = (willi_stop_btn_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(!btn) return;

    if(code == LV_EVENT_PRESSED) {
        willi_stop_btn_set_pressed(btn, true);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        willi_stop_btn_set_pressed(btn, false);
    }
}

willi_stop_btn_t *willi_stop_btn_create(lv_obj_t *parent)
{
    willi_stop_btn_t *btn = (willi_stop_btn_t *)malloc(sizeof(willi_stop_btn_t));
    if(!btn) return NULL;

    btn->pressed = false;

    btn->root = lv_obj_create(parent);
    style_root(btn->root);

    btn->face = lv_obj_create(btn->root);
    style_face(btn->face);

    btn->icon_box = lv_obj_create(btn->face);
    lv_obj_remove_style_all(btn->icon_box);
    lv_obj_set_size(btn->icon_box, 18, 18);
    lv_obj_set_style_radius(btn->icon_box, 4, 0);
    lv_obj_set_style_bg_color(btn->icon_box, lv_color_hex(STOP_RED_SOFT), 0);
    lv_obj_set_style_bg_opa(btn->icon_box, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(btn->icon_box, lv_color_hex(STOP_RED), 0);
    lv_obj_set_style_shadow_width(btn->icon_box, 8, 0);
    lv_obj_set_style_shadow_opa(btn->icon_box, LV_OPA_30, 0);
    lv_obj_align(btn->icon_box, LV_ALIGN_CENTER, -50, 0);

    btn->label = lv_label_create(btn->face);
    lv_label_set_text(btn->label, "DETENER");
    lv_obj_set_style_text_color(btn->label, lv_color_hex(STOP_TEXT), 0);
    lv_obj_set_style_text_font(btn->label, &lv_font_montserrat_26, 0);
    lv_obj_align(btn->label, LV_ALIGN_CENTER, 20, 0);

    lv_obj_add_event_cb(btn->root, event_cb, LV_EVENT_ALL, btn);
    return btn;
}

lv_obj_t *willi_stop_btn_get_root(willi_stop_btn_t *btn)
{
    return btn ? btn->root : NULL;
}

void willi_stop_btn_set_pressed(willi_stop_btn_t *btn, bool pressed)
{
    if(!btn) return;
    if(btn->pressed == pressed) return;

    btn->pressed = pressed;

    lv_anim_del(btn->face, press_y_cb);
    lv_anim_del(btn->face, press_zoom_cb);

    lv_anim_t a;

    if(pressed) {
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 0, 3);
        lv_anim_set_time(&a, 80);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, press_y_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 256, 248);
        lv_anim_set_time(&a, 80);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, press_zoom_cb);
        lv_anim_start(&a);
    } else {
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 3, 0);
        lv_anim_set_time(&a, 140);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&a, press_y_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 248, 256);
        lv_anim_set_time(&a, 140);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&a, press_zoom_cb);
        lv_anim_start(&a);
    }
}