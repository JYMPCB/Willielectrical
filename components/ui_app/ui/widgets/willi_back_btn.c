#include "willi_back_btn.h"

lv_obj_t *willi_back_btn_create(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 120, 50);

    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x151B25), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_60, 0);

    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x556070), 0);
    lv_obj_set_style_border_opa(btn, LV_OPA_40, 0);

    lv_obj_set_style_shadow_width(btn, 8, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_10, 0);
    lv_obj_set_style_shadow_color(btn, lv_color_black(), 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_LEFT "  SALIR");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);

    return btn;
}