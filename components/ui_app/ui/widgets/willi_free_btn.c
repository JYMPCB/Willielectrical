#include "../../../willi_common/include/willi_opacity.h"
#include "willi_free_btn.h"
#include <stdbool.h>

#define WILLI_FREE_BTN_W             260
#define WILLI_FREE_BTN_H              50
#define WILLI_FREE_BTN_RADIUS          6

#define WILLI_GREEN_TOP         0x6FAE1E
#define WILLI_GREEN_BOT         0x223807
#define WILLI_GREEN_EDGE        0xA4FF39
#define WILLI_GREEN_SHADOW      0x79FF12
#define WILLI_TEXT_MAIN         0xF7F7F7

#define WILLI_FREE_ICON         LV_SYMBOL_PLAY

static void willi_free_btn_init_style_main(lv_style_t *s)
{
    lv_style_init(s);

    lv_style_set_radius(s, WILLI_FREE_BTN_RADIUS);

    lv_style_set_bg_opa(s, OPA_100);
    lv_style_set_bg_color(s, lv_color_hex(WILLI_GREEN_TOP));
    lv_style_set_bg_grad_color(s, lv_color_hex(WILLI_GREEN_BOT));
    lv_style_set_bg_grad_dir(s, LV_GRAD_DIR_HOR);

    lv_style_set_border_width(s, 1);
    lv_style_set_border_color(s, lv_color_hex(WILLI_GREEN_EDGE));
    lv_style_set_border_opa(s, OPA_65);

    lv_style_set_shadow_width(s, 14);
    lv_style_set_shadow_spread(s, 1);
    lv_style_set_shadow_color(s, lv_color_hex(WILLI_GREEN_SHADOW));
    lv_style_set_shadow_opa(s, OPA_18);
    lv_style_set_shadow_ofs_x(s, 0);
    lv_style_set_shadow_ofs_y(s, 0);

    lv_style_set_pad_all(s, 0);
}

static void willi_free_btn_init_style_pressed(lv_style_t *s)
{
    lv_style_init(s);

    lv_style_set_bg_opa(s, OPA_100);
    lv_style_set_bg_color(s, lv_color_hex(0x84CF24));
    lv_style_set_bg_grad_color(s, lv_color_hex(0x2E4B09));
    lv_style_set_bg_grad_dir(s, LV_GRAD_DIR_HOR);

    lv_style_set_border_color(s, lv_color_hex(0xD2FF8A));
    lv_style_set_border_opa(s, OPA_80);

    lv_style_set_shadow_width(s, 18);
    lv_style_set_shadow_spread(s, 2);
    lv_style_set_shadow_color(s, lv_color_hex(0x90FF27));
    lv_style_set_shadow_opa(s, OPA_24);

    lv_style_set_translate_y(s, 1);
}

static void willi_free_btn_init_style_checked(lv_style_t *s)
{
    lv_style_init(s);

    lv_style_set_bg_opa(s, OPA_100);
    lv_style_set_bg_color(s, lv_color_hex(0x83CC22));
    lv_style_set_bg_grad_color(s, lv_color_hex(0x243C07));
    lv_style_set_bg_grad_dir(s, LV_GRAD_DIR_HOR);

    lv_style_set_border_width(s, 1);
    lv_style_set_border_color(s, lv_color_hex(0xC9FF72));
    lv_style_set_border_opa(s, OPA_85);

    lv_style_set_shadow_width(s, 20);
    lv_style_set_shadow_spread(s, 2);
    lv_style_set_shadow_color(s, lv_color_hex(0x8CFF24));
    lv_style_set_shadow_opa(s, OPA_28);
}

static void willi_free_btn_init_style_topline(lv_style_t *s)
{
    lv_style_init(s);
    lv_style_set_bg_opa(s, OPA_20);
    lv_style_set_bg_color(s, lv_color_hex(0xFFFFFF));
    lv_style_set_radius(s, 2);
    lv_style_set_border_width(s, 0);
    lv_style_set_pad_all(s, 0);
}

lv_obj_t *willi_free_btn_create(lv_obj_t *parent)
{
    static lv_style_t style_main;
    static lv_style_t style_pressed;
    static lv_style_t style_checked;
    static lv_style_t style_topline;
    static bool inited = false;

    if(!inited) {
        willi_free_btn_init_style_main(&style_main);
        willi_free_btn_init_style_pressed(&style_pressed);
        willi_free_btn_init_style_checked(&style_checked);
        willi_free_btn_init_style_topline(&style_topline);
        inited = true;
    }

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);

    lv_obj_add_style(btn, &style_main, LV_PART_MAIN);
    lv_obj_add_style(btn, &style_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(btn, &style_checked, LV_PART_MAIN | LV_STATE_CHECKED);

    lv_obj_set_size(btn, WILLI_FREE_BTN_W, WILLI_FREE_BTN_H);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_clip_corner(btn, true, 0);

    lv_obj_t *topline = lv_obj_create(btn);
    lv_obj_remove_style_all(topline);
    lv_obj_add_style(topline, &style_topline, 0);
    lv_obj_set_size(topline, WILLI_FREE_BTN_W - 20, 2);
    lv_obj_align(topline, LV_ALIGN_TOP_MID, 0, 3);
    lv_obj_clear_flag(topline, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(topline, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_t *content = lv_obj_create(btn);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_center(content);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(content, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(content, 6, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);

    lv_obj_t *icon = lv_label_create(content);
    lv_label_set_text(icon, WILLI_FREE_ICON);
    lv_obj_set_style_text_color(icon, lv_color_hex(WILLI_TEXT_MAIN), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);

    lv_obj_t *label = lv_label_create(content);
    lv_label_set_text(label, "LIBRE");
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(label, lv_color_hex(WILLI_TEXT_MAIN), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);

    return btn;
}