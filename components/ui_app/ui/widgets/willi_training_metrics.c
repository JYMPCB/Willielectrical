#include "../../../willi_common/include/willi_opacity.h"
#include "willi_training_metrics.h"
#include <math.h>
#include "lvgl/src/misc/lv_timer_private.h"

#define C_TITLE        0xEAEAEA
#define C_WHITE        0xF2F2F2
#define C_GREEN        0x8DFF2F
#define C_GREEN_SOFT   0x5CFF72
#define C_BLUE         0x61B8FF
#define C_BLUE_SOFT    0x37D8FF
#define C_ORANGE       0xFFB12A
#define C_LINE         0xAEB7C8
#define C_PANEL_BORDER 0xAAB4C8
#define C_WIFI         0xEAF3FF

#define TOP_Y          88
#define CARD_W         220
#define CARD_H         158

#define CARD_1_X       48
#define CARD_2_X       275
#define CARD_3_X       502
#define CARD_4_X       729

#define SEP_Y          108
#define SEP_H          128
#define SEP_1_X        249
#define SEP_2_X        476
#define SEP_3_X        703

#define TIME_PANEL_X   360
#define TIME_PANEL_Y   300
#define TIME_PANEL_W   300
#define TIME_PANEL_H   74

#define CURVE_LEFT_X      90
#define CURVE_RIGHT_X     910
#define CURVE_BASE_Y      390
#define CURVE_ARCH_H      -34

#define CURVE2_LEFT_X     150
#define CURVE2_RIGHT_X    850
#define CURVE2_BASE_Y     400
#define CURVE2_ARCH_H     -24

#define CURVE3_LEFT_X     260
#define CURVE3_RIGHT_X    740
#define CURVE3_BASE_Y     410
#define CURVE3_ARCH_H     -14

#define WIFI_X            865
#define WIFI_Y            20

static lv_style_t s_curve1_style;
static lv_style_t s_curve1_glow_style;
static lv_style_t s_curve2_style;
static lv_style_t s_curve3_style;
static bool s_styles_init = false;

static void setup_card(willi_metric_card_t *card,
                       const char *title,
                       const char *value,
                       const char *unit,
                       uint32_t value_color)
{
    willi_metric_card_set_title(card, title);
    willi_metric_card_set_value(card, value);
    willi_metric_card_set_unit(card, unit);
    willi_metric_card_set_title_color(card, lv_color_hex(C_TITLE));
    willi_metric_card_set_unit_color(card, lv_color_hex(C_TITLE));
    willi_metric_card_set_value_color(card, lv_color_hex(value_color));
}

static void style_separator(lv_obj_t *obj)
{
    lv_obj_set_size(obj, 1, SEP_H);
    lv_obj_set_style_bg_color(obj, lv_color_hex(C_LINE), 0);
    lv_obj_set_style_bg_opa(obj, OPA_24, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void style_time_panel(lv_obj_t *obj)
{
    lv_obj_set_size(obj, TIME_PANEL_W, TIME_PANEL_H);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(obj, OPA_10, 0);

    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(C_PANEL_BORDER), 0);
    lv_obj_set_style_border_opa(obj, OPA_24, 0);

    lv_obj_set_style_radius(obj, 18, 0);

    lv_obj_set_style_shadow_width(obj, 20, 0);
    lv_obj_set_style_shadow_spread(obj, 0, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_shadow_opa(obj, OPA_8, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void place_top_card(willi_metric_card_t *card, lv_coord_t x, lv_coord_t y)
{
    willi_metric_card_set_pos(card, x, y);
    willi_metric_card_set_size(card, CARD_W, CARD_H);
}

static int32_t lerp_i32(int32_t a, int32_t b, int32_t t, int32_t den)
{
    return a + ((b - a) * t) / den;
}

static void build_quadratic_curve(willi_curve_point_t *pts,
                                  uint16_t count,
                                  lv_coord_t x0, lv_coord_t y0,
                                  lv_coord_t cx, lv_coord_t cy,
                                  lv_coord_t x1, lv_coord_t y1)
{
    if(!pts || count < 2) return;

    uint16_t den = count - 1;

    for(uint16_t i = 0; i < count; i++) {
        int32_t t = i;

        int32_t xa = lerp_i32(x0, cx, t, den);
        int32_t ya = lerp_i32(y0, cy, t, den);

        int32_t xb = lerp_i32(cx, x1, t, den);
        int32_t yb = lerp_i32(cy, y1, t, den);

        pts[i].x = lerp_i32(xa, xb, t, den);
        pts[i].y = lerp_i32(ya, yb, t, den);
    }
}

static void ensure_curve_styles(void)
{
    if(s_styles_init) return;

    lv_style_init(&s_curve1_style);
    lv_style_set_line_width(&s_curve1_style, 4);
    lv_style_set_line_color(&s_curve1_style, lv_color_hex(C_GREEN_SOFT));
    lv_style_set_line_opa(&s_curve1_style, OPA_78);
    lv_style_set_line_rounded(&s_curve1_style, true);

    lv_style_init(&s_curve1_glow_style);
    lv_style_set_line_width(&s_curve1_glow_style, 10);
    lv_style_set_line_color(&s_curve1_glow_style, lv_color_hex(C_GREEN_SOFT));
    lv_style_set_line_opa(&s_curve1_glow_style, OPA_12);
    lv_style_set_line_rounded(&s_curve1_glow_style, true);

    lv_style_init(&s_curve2_style);
    lv_style_set_line_width(&s_curve2_style, 3);
    lv_style_set_line_color(&s_curve2_style, lv_color_hex(C_BLUE_SOFT));
    lv_style_set_line_opa(&s_curve2_style, OPA_42);
    lv_style_set_line_rounded(&s_curve2_style, true);

    lv_style_init(&s_curve3_style);
    lv_style_set_line_width(&s_curve3_style, 2);
    lv_style_set_line_color(&s_curve3_style, lv_color_hex(C_GREEN_SOFT));
    lv_style_set_line_opa(&s_curve3_style, OPA_16);
    lv_style_set_line_rounded(&s_curve3_style, true);

    s_styles_init = true;
}

static void style_travel_dot(lv_obj_t *obj, lv_color_t color, lv_coord_t size, lv_opa_t opa)
{
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_set_style_shadow_width(obj, 24, 0);
    lv_obj_set_style_shadow_spread(obj, 2, 0);
    lv_obj_set_style_shadow_color(obj, color, 0);
    lv_obj_set_style_shadow_opa(obj, opa, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void style_dot_halo(lv_obj_t *obj, lv_color_t color, lv_coord_t size, lv_opa_t opa)
{
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static lv_point_t interp_curve_point(const willi_curve_point_t *pts, uint16_t count, float pos)
{
    lv_point_t out = {0, 0};
    if(!pts || count < 2) return out;

    float max_pos = (float)(count - 1);

    while(pos >= max_pos) pos -= max_pos;
    while(pos < 0.0f) pos += max_pos;

    {
        int idx = (int)pos;
        float frac = pos - (float)idx;
        int next = idx + 1;
        if(next >= count) next = 0;

        out.x = (lv_coord_t)((float)pts[idx].x + ((float)pts[next].x - (float)pts[idx].x) * frac);
        out.y = (lv_coord_t)((float)pts[idx].y + ((float)pts[next].y - (float)pts[idx].y) * frac);
    }

    return out;
}

static void set_hidden(lv_obj_t *obj, bool hidden)
{
    if(!obj) return;

    if(hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void place_dot_pair(lv_obj_t *halo, lv_obj_t *dot, lv_point_t p, lv_coord_t halo_r, lv_coord_t dot_r)
{
    if(halo) lv_obj_set_pos(halo, p.x - halo_r, p.y - halo_r);
    if(dot)  lv_obj_set_pos(dot,  p.x - dot_r,  p.y - dot_r);
}

static void wifi_apply_phase(willi_training_metrics_t *m)
{
    lv_opa_t opa = OPA_36;

    if(!m || !m->wifi_label) return;

    switch(m->wifi_phase) {
        case 0: opa = OPA_28; break;
        case 1: opa = OPA_44; break;
        case 2: opa = OPA_60; break;
        case 3: opa = OPA_90; break;
        case 4: opa = OPA_60; break;
        default: opa = OPA_40; break;
    }

    lv_obj_set_style_text_opa(m->wifi_label, opa, 0);
}

static void wifi_timer_cb(lv_timer_t *t)
{
    willi_training_metrics_t *m = (willi_training_metrics_t *)t->user_data;
    if(!m) return;

    m->wifi_phase = (m->wifi_phase + 1) % 6;
    wifi_apply_phase(m);
}

static void curve_timer_cb(lv_timer_t *t)
{
    willi_training_metrics_t *m = (willi_training_metrics_t *)t->user_data;
    if(!m) return;

    m->dot_pos_1 += 0.34f;
    m->dot_pos_2 += 0.22f;

    {
        lv_point_t p1 = interp_curve_point(m->curve1_pts, WILLI_CURVE_PTS, m->dot_pos_1);
        lv_point_t p2 = interp_curve_point(m->curve2_pts, WILLI_CURVE_PTS, m->dot_pos_2);

        place_dot_pair(m->travel_dot_1_halo, m->travel_dot_1, p1, 8, 4);
        place_dot_pair(m->travel_dot_2_halo, m->travel_dot_2, p2, 6, 3);
    }
}

static void create_wifi_icon(lv_obj_t *parent, willi_training_metrics_t *m)
{
    m->wifi_label = lv_label_create(parent);
    lv_label_set_text(m->wifi_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(m->wifi_label, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(m->wifi_label, lv_color_hex(C_WIFI), 0);
    lv_obj_set_style_text_opa(m->wifi_label, OPA_60, 0);
    lv_obj_set_pos(m->wifi_label, WIFI_X, WIFI_Y);
    lv_obj_clear_flag(m->wifi_label, LV_OBJ_FLAG_CLICKABLE);

    m->wifi_phase = 0;
    wifi_apply_phase(m);
}

static void setup_line_object(lv_obj_t *line, lv_obj_t *parent)
{
    lv_coord_t w;
    lv_coord_t h;

    if(!line || !parent) return;

    w = lv_obj_get_width(parent);
    h = lv_obj_get_height(parent);

    if(w <= 0) w = 1024;
    if(h <= 0) h = 600;

    lv_obj_set_pos(line, 0, 0);
    lv_obj_set_size(line, w, h);

    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
}

void willi_training_metrics_create(lv_obj_t *parent, willi_training_metrics_t *m)
{
    if(!parent || !m) return;

    ensure_curve_styles();

    m->curve_timer = NULL;
    m->wifi_timer = NULL;
    m->dot_pos_1 = 4.0f;
    m->dot_pos_2 = 24.0f;
    m->wifi_phase = 0;

    willi_metric_card_create(&m->speed, parent);
    place_top_card(&m->speed, CARD_1_X, TOP_Y);
    setup_card(&m->speed, "VELOCIDAD", "8.5", "km/h", C_GREEN);

    willi_metric_card_create(&m->pace, parent);
    place_top_card(&m->pace, CARD_2_X, TOP_Y);
    setup_card(&m->pace, "RITMO", "07:03", "min/km", C_BLUE);

    willi_metric_card_create(&m->distance, parent);
    place_top_card(&m->distance, CARD_3_X, TOP_Y);
    setup_card(&m->distance, "DISTANCIA", "2.54", "km", C_WHITE);

    willi_metric_card_create(&m->calories, parent);
    place_top_card(&m->calories, CARD_4_X, TOP_Y);
    setup_card(&m->calories, "CALORÍAS", "174", "kcal", C_ORANGE);

    willi_metric_card_create(&m->time, parent);
    willi_metric_card_set_pos(&m->time, TIME_PANEL_X, TIME_PANEL_Y);
    willi_metric_card_set_size(&m->time, TIME_PANEL_W, TIME_PANEL_H);
    setup_card(&m->time, "", "00:18:33", "", C_WHITE);

    if(m->time.label_title) lv_obj_add_flag(m->time.label_title, LV_OBJ_FLAG_HIDDEN);
    if(m->time.label_unit)  lv_obj_add_flag(m->time.label_unit, LV_OBJ_FLAG_HIDDEN);

    if(m->time.label_value) {
        lv_obj_set_width(m->time.label_value, LV_PCT(100));
        lv_obj_set_style_text_align(m->time.label_value, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(m->time.label_value, &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(m->time.label_value, lv_color_hex(C_WHITE), 0);
        lv_obj_center(m->time.label_value);
    }

    m->time_panel = lv_obj_create(parent);
    style_time_panel(m->time_panel);
    lv_obj_set_pos(m->time_panel, TIME_PANEL_X, TIME_PANEL_Y);

    m->sep_1 = lv_obj_create(parent);
    style_separator(m->sep_1);
    lv_obj_set_pos(m->sep_1, SEP_1_X, SEP_Y);

    m->sep_2 = lv_obj_create(parent);
    style_separator(m->sep_2);
    lv_obj_set_pos(m->sep_2, SEP_2_X, SEP_Y);

    m->sep_3 = lv_obj_create(parent);
    style_separator(m->sep_3);
    lv_obj_set_pos(m->sep_3, SEP_3_X, SEP_Y);

    build_quadratic_curve(
        m->curve1_pts, WILLI_CURVE_PTS,
        CURVE_LEFT_X, CURVE_BASE_Y,
        (CURVE_LEFT_X + CURVE_RIGHT_X) / 2, CURVE_BASE_Y + CURVE_ARCH_H,
        CURVE_RIGHT_X, CURVE_BASE_Y
    );

    build_quadratic_curve(
        m->curve2_pts, WILLI_CURVE_PTS,
        CURVE2_LEFT_X, CURVE2_BASE_Y,
        (CURVE2_LEFT_X + CURVE2_RIGHT_X) / 2, CURVE2_BASE_Y + CURVE2_ARCH_H,
        CURVE2_RIGHT_X, CURVE2_BASE_Y
    );

    build_quadratic_curve(
        m->curve3_pts, WILLI_CURVE_PTS,
        CURVE3_LEFT_X, CURVE3_BASE_Y,
        (CURVE3_LEFT_X + CURVE3_RIGHT_X) / 2, CURVE3_BASE_Y + CURVE3_ARCH_H,
        CURVE3_RIGHT_X, CURVE3_BASE_Y
    );

    m->curve_1_glow = lv_line_create(parent);
    setup_line_object(m->curve_1_glow, parent);
    lv_line_set_points(m->curve_1_glow, m->curve1_pts, WILLI_CURVE_PTS);
    lv_obj_add_style(m->curve_1_glow, &s_curve1_glow_style, LV_PART_MAIN);

    m->curve_1 = lv_line_create(parent);
    setup_line_object(m->curve_1, parent);
    lv_line_set_points(m->curve_1, m->curve1_pts, WILLI_CURVE_PTS);
    lv_obj_add_style(m->curve_1, &s_curve1_style, LV_PART_MAIN);

    m->curve_2 = lv_line_create(parent);
    setup_line_object(m->curve_2, parent);
    lv_line_set_points(m->curve_2, m->curve2_pts, WILLI_CURVE_PTS);
    lv_obj_add_style(m->curve_2, &s_curve2_style, LV_PART_MAIN);

    m->curve_3 = lv_line_create(parent);
    setup_line_object(m->curve_3, parent);
    lv_line_set_points(m->curve_3, m->curve3_pts, WILLI_CURVE_PTS);
    lv_obj_add_style(m->curve_3, &s_curve3_style, LV_PART_MAIN);

    m->travel_dot_1_halo = lv_obj_create(parent);
    style_dot_halo(m->travel_dot_1_halo, lv_color_hex(C_GREEN_SOFT), 16, OPA_20);

    m->travel_dot_1 = lv_obj_create(parent);
    style_travel_dot(m->travel_dot_1, lv_color_hex(0x7CFF6B), 8, OPA_90);

    m->travel_dot_2_halo = lv_obj_create(parent);
    style_dot_halo(m->travel_dot_2_halo, lv_color_hex(C_BLUE_SOFT), 12, OPA_18);

    m->travel_dot_2 = lv_obj_create(parent);
    style_travel_dot(m->travel_dot_2, lv_color_hex(C_BLUE_SOFT), 6, OPA_78);

    {
        lv_point_t p1 = interp_curve_point(m->curve1_pts, WILLI_CURVE_PTS, m->dot_pos_1);
        lv_point_t p2 = interp_curve_point(m->curve2_pts, WILLI_CURVE_PTS, m->dot_pos_2);

        place_dot_pair(m->travel_dot_1_halo, m->travel_dot_1, p1, 8, 4);
        place_dot_pair(m->travel_dot_2_halo, m->travel_dot_2, p2, 6, 3);
    }

    create_wifi_icon(parent, m);

    lv_obj_move_foreground(m->curve_3);
    lv_obj_move_foreground(m->curve_2);
    lv_obj_move_foreground(m->curve_1_glow);
    lv_obj_move_foreground(m->curve_1);
    lv_obj_move_foreground(m->travel_dot_1_halo);
    lv_obj_move_foreground(m->travel_dot_2_halo);
    lv_obj_move_foreground(m->travel_dot_1);
    lv_obj_move_foreground(m->travel_dot_2);
    lv_obj_move_foreground(m->time_panel);
    lv_obj_move_foreground(m->time.root);
    lv_obj_move_foreground(m->wifi_label);
}

void willi_training_metrics_hide_all(willi_training_metrics_t *m)
{
    if(!m) return;

    willi_metric_card_hide(&m->speed);
    willi_metric_card_hide(&m->pace);
    willi_metric_card_hide(&m->distance);
    willi_metric_card_hide(&m->calories);
    willi_metric_card_hide(&m->time);

    set_hidden(m->sep_1, true);
    set_hidden(m->sep_2, true);
    set_hidden(m->sep_3, true);
    set_hidden(m->time_panel, true);

    set_hidden(m->curve_1_glow, true);
    set_hidden(m->curve_1, true);
    set_hidden(m->curve_2, true);
    set_hidden(m->curve_3, true);

    set_hidden(m->travel_dot_1_halo, true);
    set_hidden(m->travel_dot_1, true);
    set_hidden(m->travel_dot_2_halo, true);
    set_hidden(m->travel_dot_2, true);

    set_hidden(m->wifi_label, true);

    if(m->curve_timer) {
        lv_timer_del(m->curve_timer);
        m->curve_timer = NULL;
    }

    if(m->wifi_timer) {
        lv_timer_del(m->wifi_timer);
        m->wifi_timer = NULL;
    }
}

void willi_training_metrics_show_animated(willi_training_metrics_t *m)
{
    if(!m) return;

    set_hidden(m->sep_1, false);
    set_hidden(m->sep_2, false);
    set_hidden(m->sep_3, false);
    set_hidden(m->time_panel, false);

    set_hidden(m->curve_1_glow, false);
    set_hidden(m->curve_1, false);
    set_hidden(m->curve_2, false);
    set_hidden(m->curve_3, false);

    set_hidden(m->travel_dot_1_halo, false);
    set_hidden(m->travel_dot_1, false);
    set_hidden(m->travel_dot_2_halo, false);
    set_hidden(m->travel_dot_2, false);

    set_hidden(m->wifi_label, false);

    willi_metric_card_prepare_in_anim(&m->speed, 24);
    willi_metric_card_prepare_in_anim(&m->pace, 24);
    willi_metric_card_prepare_in_anim(&m->distance, 24);
    willi_metric_card_prepare_in_anim(&m->calories, 24);
    willi_metric_card_prepare_in_anim(&m->time, 18);

    willi_metric_card_animate_in(&m->speed, 0);
    willi_metric_card_animate_in(&m->pace, 70);
    willi_metric_card_animate_in(&m->distance, 140);
    willi_metric_card_animate_in(&m->calories, 210);
    willi_metric_card_animate_in(&m->time, 260);

    lv_obj_move_foreground(m->curve_3);
    lv_obj_move_foreground(m->curve_2);
    lv_obj_move_foreground(m->curve_1_glow);
    lv_obj_move_foreground(m->curve_1);
    lv_obj_move_foreground(m->travel_dot_1_halo);
    lv_obj_move_foreground(m->travel_dot_2_halo);
    lv_obj_move_foreground(m->travel_dot_1);
    lv_obj_move_foreground(m->travel_dot_2);
    lv_obj_move_foreground(m->time_panel);
    lv_obj_move_foreground(m->time.root);
    lv_obj_move_foreground(m->wifi_label);

    if(!m->curve_timer) {
        m->curve_timer = lv_timer_create(curve_timer_cb, 22, m);
    }

    if(!m->wifi_timer) {
        m->wifi_timer = lv_timer_create(wifi_timer_cb, 180, m);
    }
}

void willi_training_metrics_hide_animated(willi_training_metrics_t *m)
{
    if(!m) return;

    willi_metric_card_prepare_out_anim(&m->time, 0);
    willi_metric_card_prepare_out_anim(&m->calories, 0);
    willi_metric_card_prepare_out_anim(&m->distance, 0);
    willi_metric_card_prepare_out_anim(&m->pace, 0);
    willi_metric_card_prepare_out_anim(&m->speed, 0);

    willi_metric_card_animate_out(&m->time, 0);
    willi_metric_card_animate_out(&m->calories, 40);
    willi_metric_card_animate_out(&m->distance, 80);
    willi_metric_card_animate_out(&m->pace, 120);
    willi_metric_card_animate_out(&m->speed, 160);

    if(m->curve_timer) {
        lv_timer_del(m->curve_timer);
        m->curve_timer = NULL;
    }

    if(m->wifi_timer) {
        lv_timer_del(m->wifi_timer);
        m->wifi_timer = NULL;
    }

    set_hidden(m->sep_1, true);
    set_hidden(m->sep_2, true);
    set_hidden(m->sep_3, true);
    set_hidden(m->time_panel, true);

    set_hidden(m->curve_1_glow, true);
    set_hidden(m->curve_1, true);
    set_hidden(m->curve_2, true);
    set_hidden(m->curve_3, true);

    set_hidden(m->travel_dot_1_halo, true);
    set_hidden(m->travel_dot_1, true);
    set_hidden(m->travel_dot_2_halo, true);
    set_hidden(m->travel_dot_2, true);

    set_hidden(m->wifi_label, true);
}