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
    lv_obj_set_size(obj, 250, 68);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

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

    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1A0B0D), LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(obj, LV_OPA_80, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(obj, 6, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_10, LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(obj, 2, LV_STATE_PRESSED);

    lv_obj_set_style_pad_all(obj, 0, 0);
}

willi_stop_btn_t *willi_stop_btn_create(lv_obj_t *parent)
{
    willi_stop_btn_t *btn = (willi_stop_btn_t *)malloc(sizeof(willi_stop_btn_t));
    if(!btn) return NULL;

    btn->pressed = false;

    btn->root = lv_btn_create(parent);
    style_root(btn->root);

    btn->face = btn->root;

    btn->icon_box = lv_obj_create(btn->root);
    lv_obj_remove_style_all(btn->icon_box);
    lv_obj_clear_flag(btn->icon_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn->icon_box, 18, 18);
    lv_obj_set_style_radius(btn->icon_box, 4, 0);
    lv_obj_set_style_bg_color(btn->icon_box, lv_color_hex(STOP_RED_SOFT), 0);
    lv_obj_set_style_bg_opa(btn->icon_box, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(btn->icon_box, lv_color_hex(STOP_RED), 0);
    lv_obj_set_style_shadow_width(btn->icon_box, 8, 0);
    lv_obj_set_style_shadow_opa(btn->icon_box, LV_OPA_30, 0);
    lv_obj_set_style_bg_opa(btn->icon_box, LV_OPA_70, LV_STATE_PRESSED);
    lv_obj_align(btn->icon_box, LV_ALIGN_CENTER, -68, 0);

    btn->label = lv_label_create(btn->root);
    lv_label_set_text(btn->label, "DETENER");
    lv_obj_clear_flag(btn->label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_color(btn->label, lv_color_hex(STOP_TEXT), 0);
    lv_obj_set_style_text_font(btn->label, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_opa(btn->label, LV_OPA_70, LV_STATE_PRESSED);
    lv_obj_align(btn->label, LV_ALIGN_CENTER, 26, 0);

    return btn;
}

lv_obj_t *willi_stop_btn_get_root(willi_stop_btn_t *btn)
{
    return btn ? btn->root : NULL;
}

void willi_stop_btn_set_pressed(willi_stop_btn_t *btn, bool pressed)
{
    (void)btn;
    (void)pressed;
}