#include "../../willi_common/include/willi_opacity.h"
#include "willi_training_metrics.h"
#include "lvgl/src/misc/lv_timer_private.h"

#define C_TITLE        0xEAEAEA
#define C_WHITE        0xF2F2F2
#define C_GREEN        0x8DFF2F
#define C_BLUE         0x61B8FF
#define C_ORANGE       0xFFB12A
#define C_LINE         0xAEB7C8
#define C_PANEL_BORDER 0xAAB4C8

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
#define TIME_PANEL_Y   315
#define TIME_PANEL_W   300
#define TIME_PANEL_H   74

#define CURVE_LEFT_X      90
#define CURVE_RIGHT_X     910
#define CURVE_BASE_Y      375
#define CURVE_ARCH_H      26

#define CURVE2_LEFT_X     150
#define CURVE2_RIGHT_X    850
#define CURVE2_BASE_Y     388
#define CURVE2_ARCH_H     18

#define CURVE3_LEFT_X     220
#define CURVE3_RIGHT_X    780
#define CURVE3_BASE_Y     401
#define CURVE3_ARCH_H     10

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
    lv_obj_set_style_bg_opa(obj, OPA_12, 0);

    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(C_PANEL_BORDER), 0);
    lv_obj_set_style_border_opa(obj, OPA_18, 0);

    lv_obj_set_style_radius(obj, 16, 0);

    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_0, 0);

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

static void build_quadratic_curve(lv_point_t *pts,
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

        pts[i].x = (lv_coord_t)lerp_i32(xa, xb, t, den);
        pts[i].y = (lv_coord_t)lerp_i32(ya, yb, t, den);
    }
}

static void style_travel_dot(lv_obj_t *obj, lv_color_t color, lv_coord_t size, lv_opa_t opa)
{
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_set_style_shadow_width(obj, 26, 0);
    lv_obj_set_style_shadow_spread(obj, 2, 0);
    lv_obj_set_style_shadow_color(obj, color, 0);
    lv_obj_set_style_shadow_opa(obj, opa, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void style_curve_seg(lv_obj_t *obj, lv_color_t color, lv_opa_t opa, lv_coord_t thickness, lv_coord_t w)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, w, thickness);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_set_style_shadow_width(obj, thickness * 4, 0);
    lv_obj_set_style_shadow_spread(obj, 1, 0);
    lv_obj_set_style_shadow_color(obj, color, 0);
    lv_obj_set_style_shadow_opa(obj, opa, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static lv_coord_t abs_i(lv_coord_t v)
{
    return (v < 0) ? -v : v;
}

static void build_curve_segments(lv_obj_t *parent,
                                 lv_obj_t **segs,
                                 const lv_point_t *pts,
                                 uint16_t count,
                                 lv_color_t color,
                                 lv_opa_t opa,
                                 lv_coord_t thickness)
{
    if(!parent || !segs || !pts || count < 2) return;

    for(uint16_t i = 0; i < count - 1; i++) {
        lv_coord_t x1 = pts[i].x;
        lv_coord_t y1 = pts[i].y;
        lv_coord_t x2 = pts[i + 1].x;
        lv_coord_t y2 = pts[i + 1].y;

        lv_coord_t seg_w = abs_i(x2 - x1);
        if(seg_w < 8) seg_w = 8;

        lv_coord_t seg_h = thickness;
        lv_coord_t px = (x1 < x2) ? x1 : x2;
        lv_coord_t py = (y1 + y2) / 2 - seg_h / 2;

        segs[i] = lv_obj_create(parent);
        style_curve_seg(segs[i], color, opa, thickness, seg_w + 2);
        lv_obj_set_pos(segs[i], px, py);
    }
}

static void set_curve_segments_hidden(lv_obj_t **segs, bool hidden)
{
    if(!segs) return;

    for(uint16_t i = 0; i < WILLI_CURVE_SEGS; i++) {
        if(!segs[i]) continue;
        if(hidden) lv_obj_add_flag(segs[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(segs[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void move_curve_segments_foreground(lv_obj_t **segs)
{
    if(!segs) return;

    for(uint16_t i = 0; i < WILLI_CURVE_SEGS; i++) {
        if(segs[i]) lv_obj_move_foreground(segs[i]);
    }
}

static void curve_timer_cb(lv_timer_t *t)
{
    willi_training_metrics_t *m = (willi_training_metrics_t *)t->user_data;
    if(!m) return;

    m->dot_idx_1 = (m->dot_idx_1 + 1) % WILLI_CURVE_PTS;
    m->dot_idx_2 = (m->dot_idx_2 + 1) % WILLI_CURVE_PTS;

    if(m->travel_dot_1) {
        lv_point_t p = m->curve1_pts[m->dot_idx_1];
        lv_obj_set_pos(m->travel_dot_1, p.x - 4, p.y - 4);
    }

    if(m->travel_dot_2) {
        lv_point_t p = m->curve2_pts[m->dot_idx_2];
        lv_obj_set_pos(m->travel_dot_2, p.x - 3, p.y - 3);
    }
}

void willi_training_metrics_create(lv_obj_t *parent, willi_training_metrics_t *m)
{
    if(!m) return;

    for(uint16_t i = 0; i < WILLI_CURVE_SEGS; i++) {
        m->curve_1_segs[i] = NULL;
        m->curve_2_segs[i] = NULL;
        m->curve_3_segs[i] = NULL;
    }

    m->curve_timer = NULL;
    m->dot_idx_1 = 4;
    m->dot_idx_2 = 24;

    /* cards superiores */
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

    /* tiempo abajo */
    willi_metric_card_create(&m->time, parent);
    willi_metric_card_set_pos(&m->time, TIME_PANEL_X, TIME_PANEL_Y);
    willi_metric_card_set_size(&m->time, TIME_PANEL_W, TIME_PANEL_H);
    setup_card(&m->time, "", "00:18:33", "", C_WHITE);

    if(m->time.label_title) lv_obj_add_flag(m->time.label_title, LV_OBJ_FLAG_HIDDEN);
    if(m->time.label_unit)  lv_obj_add_flag(m->time.label_unit,  LV_OBJ_FLAG_HIDDEN);

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

    /* separadores */
    m->sep_1 = lv_obj_create(parent);
    style_separator(m->sep_1);
    lv_obj_set_pos(m->sep_1, SEP_1_X, SEP_Y);

    m->sep_2 = lv_obj_create(parent);
    style_separator(m->sep_2);
    lv_obj_set_pos(m->sep_2, SEP_2_X, SEP_Y);

    m->sep_3 = lv_obj_create(parent);
    style_separator(m->sep_3);
    lv_obj_set_pos(m->sep_3, SEP_3_X, SEP_Y);

    /* curvas */
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

    build_curve_segments(parent, m->curve_1_segs, m->curve1_pts, WILLI_CURVE_PTS, lv_color_hex(0x5CFF72), OPA_70, 4);
    build_curve_segments(parent, m->curve_2_segs, m->curve2_pts, WILLI_CURVE_PTS, lv_color_hex(0x37D8FF), OPA_48, 3);
    build_curve_segments(parent, m->curve_3_segs, m->curve3_pts, WILLI_CURVE_PTS, lv_color_hex(0x5CFF72), OPA_20, 2);

    /* dots viajeros */
    m->travel_dot_1 = lv_obj_create(parent);
    style_travel_dot(m->travel_dot_1, lv_color_hex(0x7CFF6B), 8, OPA_90);

    m->travel_dot_2 = lv_obj_create(parent);
    style_travel_dot(m->travel_dot_2, lv_color_hex(0x37D8FF), 6, OPA_78);

    lv_obj_set_pos(m->travel_dot_1, m->curve1_pts[m->dot_idx_1].x - 4, m->curve1_pts[m->dot_idx_1].y - 4);
    lv_obj_set_pos(m->travel_dot_2, m->curve2_pts[m->dot_idx_2].x - 3, m->curve2_pts[m->dot_idx_2].y - 3);

    move_curve_segments_foreground(m->curve_1_segs);
    move_curve_segments_foreground(m->curve_2_segs);
    move_curve_segments_foreground(m->curve_3_segs);
    lv_obj_move_foreground(m->travel_dot_1);
    lv_obj_move_foreground(m->travel_dot_2);
    lv_obj_move_foreground(m->time_panel);
    lv_obj_move_foreground(m->time.root);
}

void willi_training_metrics_hide_all(willi_training_metrics_t *m)
{
    if(!m) return;

    willi_metric_card_hide(&m->speed);
    willi_metric_card_hide(&m->pace);
    willi_metric_card_hide(&m->distance);
    willi_metric_card_hide(&m->calories);
    willi_metric_card_hide(&m->time);

    if(m->sep_1) lv_obj_add_flag(m->sep_1, LV_OBJ_FLAG_HIDDEN);
    if(m->sep_2) lv_obj_add_flag(m->sep_2, LV_OBJ_FLAG_HIDDEN);
    if(m->sep_3) lv_obj_add_flag(m->sep_3, LV_OBJ_FLAG_HIDDEN);

    if(m->time_panel) lv_obj_add_flag(m->time_panel, LV_OBJ_FLAG_HIDDEN);

    set_curve_segments_hidden(m->curve_1_segs, true);
    set_curve_segments_hidden(m->curve_2_segs, true);
    set_curve_segments_hidden(m->curve_3_segs, true);

    if(m->travel_dot_1) lv_obj_add_flag(m->travel_dot_1, LV_OBJ_FLAG_HIDDEN);
    if(m->travel_dot_2) lv_obj_add_flag(m->travel_dot_2, LV_OBJ_FLAG_HIDDEN);

    if(m->curve_timer) {
        lv_timer_del(m->curve_timer);
        m->curve_timer = NULL;
    }
}

void willi_training_metrics_show_animated(willi_training_metrics_t *m)
{
    if(!m) return;

    if(m->sep_1) lv_obj_clear_flag(m->sep_1, LV_OBJ_FLAG_HIDDEN);
    if(m->sep_2) lv_obj_clear_flag(m->sep_2, LV_OBJ_FLAG_HIDDEN);
    if(m->sep_3) lv_obj_clear_flag(m->sep_3, LV_OBJ_FLAG_HIDDEN);

    if(m->time_panel) lv_obj_clear_flag(m->time_panel, LV_OBJ_FLAG_HIDDEN);

    set_curve_segments_hidden(m->curve_1_segs, false);
    set_curve_segments_hidden(m->curve_2_segs, false);
    set_curve_segments_hidden(m->curve_3_segs, false);

    if(m->travel_dot_1) lv_obj_clear_flag(m->travel_dot_1, LV_OBJ_FLAG_HIDDEN);
    if(m->travel_dot_2) lv_obj_clear_flag(m->travel_dot_2, LV_OBJ_FLAG_HIDDEN);

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

    move_curve_segments_foreground(m->curve_1_segs);
    move_curve_segments_foreground(m->curve_2_segs);
    move_curve_segments_foreground(m->curve_3_segs);

    if(m->travel_dot_1) lv_obj_move_foreground(m->travel_dot_1);
    if(m->travel_dot_2) lv_obj_move_foreground(m->travel_dot_2);
    if(m->time_panel)   lv_obj_move_foreground(m->time_panel);
    if(m->time.root)    lv_obj_move_foreground(m->time.root);

    if(!m->curve_timer) {
        m->curve_timer = lv_timer_create(curve_timer_cb, 35, m);
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

    if(m->sep_1) lv_obj_add_flag(m->sep_1, LV_OBJ_FLAG_HIDDEN);
    if(m->sep_2) lv_obj_add_flag(m->sep_2, LV_OBJ_FLAG_HIDDEN);
    if(m->sep_3) lv_obj_add_flag(m->sep_3, LV_OBJ_FLAG_HIDDEN);

    if(m->time_panel) lv_obj_add_flag(m->time_panel, LV_OBJ_FLAG_HIDDEN);

    set_curve_segments_hidden(m->curve_1_segs, true);
    set_curve_segments_hidden(m->curve_2_segs, true);
    set_curve_segments_hidden(m->curve_3_segs, true);

    if(m->travel_dot_1) lv_obj_add_flag(m->travel_dot_1, LV_OBJ_FLAG_HIDDEN);
    if(m->travel_dot_2) lv_obj_add_flag(m->travel_dot_2, LV_OBJ_FLAG_HIDDEN);
}