#include "../../../willi_common/include/willi_opacity.h"
#include "willi_stepper_btn.h"
#include <stdlib.h>

#define STEPPER_BG_1        0x1A1F2A
#define STEPPER_BG_2        0x11161F
#define STEPPER_BORDER      0x5A6270
#define STEPPER_TEXT_DIM    0xC8CDD6
#define STEPPER_TEXT_WHITE  0xF3F5F7

typedef struct {
    willi_stepper_btn_t *btn;
    bool is_plus;
} stepper_zone_ctx_t;

static void style_root(willi_stepper_btn_t *btn);
static void style_side_zone(lv_obj_t *obj);
static void style_center_zone(lv_obj_t *obj);
static void refresh_colors(willi_stepper_btn_t *btn);

static void set_event_bubble_recursive(lv_obj_t *obj);

static void zone_translate_x_cb(void *var, int32_t v);
static void zone_shadow_width_cb(void *var, int32_t v);
static void zone_shadow_opa_cb(void *var, int32_t v);
static void zone_bg_opa_cb(void *var, int32_t v);
static void label_zoom_cb(void *var, int32_t v);
static void center_glow_opa_cb(void *var, int32_t v);

static void apply_zone_pressed_style(willi_stepper_btn_t *btn, lv_obj_t *zone, lv_obj_t *label, bool pressed, bool is_plus);
static void minus_event_cb(lv_event_t *e);
static void plus_event_cb(lv_event_t *e);

static stepper_zone_ctx_t s_minus_ctx[4];
static stepper_zone_ctx_t s_plus_ctx[4];
static uint8_t s_ctx_count = 0;

/* ---------------------- styles ---------------------- */

static void style_root(willi_stepper_btn_t *btn)
{
    lv_obj_remove_style_all(btn->root);

    lv_obj_set_size(btn->root, 250, 56);
    lv_obj_clear_flag(btn->root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(btn->root, 10, 0);
    lv_obj_set_style_bg_color(btn->root, lv_color_hex(STEPPER_BG_1), 0);
    lv_obj_set_style_bg_grad_color(btn->root, lv_color_hex(STEPPER_BG_2), 0);
    lv_obj_set_style_bg_grad_dir(btn->root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(btn->root, OPA_80, 0);

    lv_obj_set_style_border_width(btn->root, 1, 0);
    lv_obj_set_style_border_color(btn->root, lv_color_hex(STEPPER_BORDER), 0);
    lv_obj_set_style_border_opa(btn->root, OPA_45, 0);

    lv_obj_set_style_shadow_color(btn->root, lv_color_black(), 0);
    lv_obj_set_style_shadow_width(btn->root, 10, 0);
    lv_obj_set_style_shadow_opa(btn->root, OPA_24, 0);

    lv_obj_set_style_pad_all(btn->root, 0, 0);
    lv_obj_set_style_pad_column(btn->root, 0, 0);
    lv_obj_set_layout(btn->root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn->root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn->root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
}

static void style_side_zone(lv_obj_t *obj)
{
    lv_obj_remove_style_all(obj);

    lv_obj_set_height(obj, LV_PCT(100));
    lv_obj_set_width(obj, 54);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 10, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);

    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_TRANSP, 0);

    lv_obj_set_style_transform_zoom(obj, 256, 0);
    lv_obj_set_style_transform_pivot_x(obj, 24, 0);
    lv_obj_set_style_transform_pivot_y(obj, 24, 0);
}

static void style_center_zone(lv_obj_t *obj)
{
    lv_obj_remove_style_all(obj);

    lv_obj_set_height(obj, LV_PCT(100));
    lv_obj_set_flex_grow(obj, 1);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);

    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_TRANSP, 0);
}

static void refresh_colors(willi_stepper_btn_t *btn)
{
    if(!btn) return;

    lv_obj_set_style_text_color(btn->title_label, btn->accent, 0);

    lv_obj_set_style_shadow_color(btn->center_zone, btn->accent, 0);
    lv_obj_set_style_shadow_width(btn->center_zone, 8, 0);
    lv_obj_set_style_shadow_opa(btn->center_zone, OPA_16, 0);

    lv_obj_set_style_shadow_color(btn->minus_zone, btn->accent, 0);
    lv_obj_set_style_shadow_color(btn->plus_zone, btn->accent, 0);
}

/* ---------------------- helpers ---------------------- */

static void set_event_bubble_recursive(lv_obj_t *obj)
{
    if(!obj) return;

    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);

    uint32_t cnt = lv_obj_get_child_cnt(obj);
    for(uint32_t i = 0; i < cnt; i++) {
        set_event_bubble_recursive(lv_obj_get_child(obj, i));
    }
}

/* ---------------------- anim callbacks ---------------------- */

static void zone_translate_x_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_translate_x(obj, (lv_coord_t)v, 0);
}

static void zone_shadow_width_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_width(obj, (lv_coord_t)v, 0);
}

static void zone_shadow_opa_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_opa(obj, (lv_opa_t)v, 0);
}

static void zone_bg_opa_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)v, 0);
}

static void label_zoom_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_transform_zoom(obj, (lv_coord_t)v, 0);
}

static void center_glow_opa_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_opa(obj, (lv_opa_t)v, 0);
}

/* ---------------------- press effects ---------------------- */

static void apply_zone_pressed_style(willi_stepper_btn_t *btn, lv_obj_t *zone, lv_obj_t *label, bool pressed, bool is_plus)
{
    if(!btn || !zone || !label) return;

    lv_anim_del(zone, zone_translate_x_cb);
    lv_anim_del(zone, zone_shadow_width_cb);
    lv_anim_del(zone, zone_shadow_opa_cb);
    lv_anim_del(zone, zone_bg_opa_cb);
    lv_anim_del(label, label_zoom_cb);
    lv_anim_del(btn->center_zone, center_glow_opa_cb);

    lv_anim_t a;

    if(pressed) {
        lv_obj_set_style_bg_color(zone, btn->accent, 0);

        lv_anim_init(&a);
        lv_anim_set_var(&a, zone);
        lv_anim_set_values(&a, 0, is_plus ? 2 : -2);
        lv_anim_set_time(&a, 80);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, zone_translate_x_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, zone);
        lv_anim_set_values(&a, 0, OPA_18);
        lv_anim_set_time(&a, 80);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, zone_bg_opa_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, zone);
        lv_anim_set_values(&a, 0, 10);
        lv_anim_set_time(&a, 80);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, zone_shadow_width_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, zone);
        lv_anim_set_values(&a, 0, OPA_24);
        lv_anim_set_time(&a, 80);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, zone_shadow_opa_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, label);
        lv_anim_set_values(&a, 256, 244);
        lv_anim_set_time(&a, 80);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, label_zoom_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->center_zone);
        lv_anim_set_values(&a, OPA_16, OPA_10);
        lv_anim_set_time(&a, 80);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, center_glow_opa_cb);
        lv_anim_start(&a);
    } else {
        lv_anim_init(&a);
        lv_anim_set_var(&a, zone);
        lv_anim_set_values(&a, is_plus ? 2 : -2, 0);
        lv_anim_set_time(&a, 140);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&a, zone_translate_x_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, zone);
        lv_anim_set_values(&a, OPA_18, 0);
        lv_anim_set_time(&a, 120);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, zone_bg_opa_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, zone);
        lv_anim_set_values(&a, 10, 0);
        lv_anim_set_time(&a, 120);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, zone_shadow_width_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, zone);
        lv_anim_set_values(&a, OPA_24, 0);
        lv_anim_set_time(&a, 120);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, zone_shadow_opa_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, label);
        lv_anim_set_values(&a, 244, 256);
        lv_anim_set_time(&a, 140);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&a, label_zoom_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->center_zone);
        lv_anim_set_values(&a, OPA_10, OPA_16);
        lv_anim_set_time(&a, 120);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, center_glow_opa_cb);
        lv_anim_start(&a);
    }
}

/* ---------------------- events ---------------------- */

static void minus_event_cb(lv_event_t *e)
{
    stepper_zone_ctx_t *ctx = (stepper_zone_ctx_t *)lv_event_get_user_data(e);
    if(!ctx || !ctx->btn) return;

    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED) {
        willi_stepper_btn_set_minus_pressed(ctx->btn, true);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        willi_stepper_btn_set_minus_pressed(ctx->btn, false);
    }
}

static void plus_event_cb(lv_event_t *e)
{
    stepper_zone_ctx_t *ctx = (stepper_zone_ctx_t *)lv_event_get_user_data(e);
    if(!ctx || !ctx->btn) return;

    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED) {
        willi_stepper_btn_set_plus_pressed(ctx->btn, true);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        willi_stepper_btn_set_plus_pressed(ctx->btn, false);
    }
}

/* ---------------------- public ---------------------- */

willi_stepper_btn_t *willi_stepper_btn_create(lv_obj_t *parent)
{
    willi_stepper_btn_t *btn = (willi_stepper_btn_t *)malloc(sizeof(willi_stepper_btn_t));
    if(!btn) return NULL;

    btn->accent = lv_color_hex(0x8DFF2D);
    btn->role = WILLI_STEPPER_ROLE_SPEED;
    btn->minus_pressed = false;
    btn->plus_pressed = false;

    btn->root = lv_obj_create(parent);
    style_root(btn);

    btn->minus_zone = lv_obj_create(btn->root);
    style_side_zone(btn->minus_zone);

    btn->center_zone = lv_obj_create(btn->root);
    style_center_zone(btn->center_zone);

    btn->plus_zone = lv_obj_create(btn->root);
    style_side_zone(btn->plus_zone);

    btn->minus_label = lv_label_create(btn->minus_zone);
    lv_label_set_text(btn->minus_label, LV_SYMBOL_MINUS);
    lv_obj_set_style_text_color(btn->minus_label, lv_color_hex(STEPPER_TEXT_DIM), 0);
    lv_obj_set_style_text_font(btn->minus_label, &lv_font_montserrat_26, 0);
    lv_obj_set_style_transform_pivot_x(btn->minus_label, 8, 0);
    lv_obj_set_style_transform_pivot_y(btn->minus_label, 13, 0);
    lv_obj_set_style_transform_zoom(btn->minus_label, 256, 0);
    lv_obj_center(btn->minus_label);

    btn->title_label = lv_label_create(btn->center_zone);
    lv_label_set_text(btn->title_label, "VELOCIDAD");
    lv_obj_set_style_text_color(btn->title_label, btn->accent, 0);
    lv_obj_set_style_text_font(btn->title_label, &lv_font_montserrat_22, 0);
    lv_obj_center(btn->title_label);

    btn->plus_label = lv_label_create(btn->plus_zone);
    lv_label_set_text(btn->plus_label, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_color(btn->plus_label, lv_color_hex(STEPPER_TEXT_DIM), 0);
    lv_obj_set_style_text_font(btn->plus_label, &lv_font_montserrat_26, 0);
    lv_obj_set_style_transform_pivot_x(btn->plus_label, 8, 0);
    lv_obj_set_style_transform_pivot_y(btn->plus_label, 13, 0);
    lv_obj_set_style_transform_zoom(btn->plus_label, 256, 0);
    lv_obj_center(btn->plus_label);

    refresh_colors(btn);

    set_event_bubble_recursive(btn->root);

    if(s_ctx_count < 4) {
        s_minus_ctx[s_ctx_count].btn = btn;
        s_minus_ctx[s_ctx_count].is_plus = false;
        lv_obj_add_event_cb(btn->minus_zone, minus_event_cb, LV_EVENT_ALL, &s_minus_ctx[s_ctx_count]);

        s_plus_ctx[s_ctx_count].btn = btn;
        s_plus_ctx[s_ctx_count].is_plus = true;
        lv_obj_add_event_cb(btn->plus_zone, plus_event_cb, LV_EVENT_ALL, &s_plus_ctx[s_ctx_count]);

        s_ctx_count++;
    }

    return btn;
}

void willi_stepper_btn_set_title(willi_stepper_btn_t *btn, const char *txt)
{
    if(!btn || !btn->title_label) return;
    lv_label_set_text(btn->title_label, txt);
}

void willi_stepper_btn_set_accent(willi_stepper_btn_t *btn, lv_color_t color)
{
    if(!btn) return;
    btn->accent = color;
    refresh_colors(btn);
}

void willi_stepper_btn_set_role(willi_stepper_btn_t *btn, willi_stepper_role_t role)
{
    if(!btn) return;
    btn->role = role;
}

void willi_stepper_btn_set_minus_pressed(willi_stepper_btn_t *btn, bool pressed)
{
    if(!btn) return;
    if(btn->minus_pressed == pressed) return;

    btn->minus_pressed = pressed;
    apply_zone_pressed_style(btn, btn->minus_zone, btn->minus_label, pressed, false);
}

void willi_stepper_btn_set_plus_pressed(willi_stepper_btn_t *btn, bool pressed)
{
    if(!btn) return;
    if(btn->plus_pressed == pressed) return;

    btn->plus_pressed = pressed;
    apply_zone_pressed_style(btn, btn->plus_zone, btn->plus_label, pressed, true);
}

lv_obj_t *willi_stepper_btn_get_root(willi_stepper_btn_t *btn)
{
    return btn ? btn->root : NULL;
}

lv_obj_t *willi_stepper_btn_get_minus_zone(willi_stepper_btn_t *btn)
{
    return btn ? btn->minus_zone : NULL;
}

lv_obj_t *willi_stepper_btn_get_plus_zone(willi_stepper_btn_t *btn)
{
    return btn ? btn->plus_zone : NULL;
}