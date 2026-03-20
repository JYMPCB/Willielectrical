#include "../../../willi_common/include/willi_opacity.h"
#include "willi_training_metrics.h"
#include "../../assets/images/img_icon_metric_speed.h"
#include "../../assets/images/img_icon_metric_pace.h"
#include "../../assets/images/img_icon_metric_distance.h"
#include "../../assets/images/img_icon_metric_calories.h"


/* assets */
LV_IMG_DECLARE(img_icon_metric_speed);
LV_IMG_DECLARE(img_icon_metric_pace);
LV_IMG_DECLARE(img_icon_metric_distance);
LV_IMG_DECLARE(img_icon_metric_calories);

#define C_TITLE         0xEAEAEA
#define C_WHITE         0xF2F2F2
#define C_GREEN         0x8DFF2F
#define C_BLUE          0x61B8FF
#define C_ORANGE        0xFFB12A
#define C_LINE          0xAEB7C8
#define C_PANEL_BORDER  0xAAB4C8

#define TOP_Y           140
#define CARD_W          220
#define CARD_H          158

#define CARD_1_X        48
#define CARD_2_X        275
#define CARD_3_X        502
#define CARD_4_X        729

#define SEP_Y           118
#define SEP_H           128
#define SEP_1_X         249
#define SEP_2_X         476
#define SEP_3_X         703

#define TIME_PANEL_X    352
#define TIME_PANEL_Y    298
#define TIME_PANEL_W    320
#define TIME_PANEL_H    74

#define FX_LEFT_X           86
#define FX_RIGHT_X          916
#define WAVE_GREEN_BASE_Y   360
#define WAVE_GREEN_DEPTH     28
#define WAVE_BLUE_BASE_Y    372
#define WAVE_BLUE_DEPTH      18

static void set_hidden(lv_obj_t *obj, bool hidden)
{
    if(!obj) return;
    if(hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

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

static void place_top_card(willi_metric_card_t *card, lv_coord_t x, lv_coord_t y)
{
    willi_metric_card_set_pos(card, x, y);
    willi_metric_card_set_size(card, CARD_W, CARD_H);
}

static void style_separator(lv_obj_t *obj)
{
    lv_obj_set_size(obj, 1, SEP_H);
    lv_obj_set_style_bg_color(obj, lv_color_hex(C_LINE), 0);
    lv_obj_set_style_bg_opa(obj, OPA_18, 0);
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

    lv_obj_set_style_shadow_width(obj, 24, 0);
    lv_obj_set_style_shadow_spread(obj, 0, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_shadow_opa(obj, OPA_10, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void create_metric_icons(lv_obj_t *parent, willi_training_metrics_t *m)
{
    if(!parent || !m) return;

    m->icon_speed = lv_img_create(parent);
    lv_img_set_src(m->icon_speed, &img_icon_metric_speed);

    m->icon_pace = lv_img_create(parent);
    lv_img_set_src(m->icon_pace, &img_icon_metric_pace);

    m->icon_distance = lv_img_create(parent);
    lv_img_set_src(m->icon_distance, &img_icon_metric_distance);

    m->icon_calories = lv_img_create(parent);
    lv_img_set_src(m->icon_calories, &img_icon_metric_calories);

    lv_obj_clear_flag(m->icon_speed, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(m->icon_pace, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(m->icon_distance, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(m->icon_calories, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_align_to(m->icon_speed,    m->speed.root,    LV_ALIGN_TOP_MID, 0, -60);
    lv_obj_align_to(m->icon_pace,     m->pace.root,     LV_ALIGN_TOP_MID, 0, -65);
    lv_obj_align_to(m->icon_distance, m->distance.root, LV_ALIGN_TOP_MID, 0, -75);
    lv_obj_align_to(m->icon_calories, m->calories.root, LV_ALIGN_TOP_MID, 0, -75);
}

void willi_training_metrics_create(lv_obj_t *parent, willi_training_metrics_t *m)
{
    if(!parent || !m) return;

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
    setup_card(&m->calories, "CALORIAS", "174", "kcal", C_ORANGE);

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

    m->sep_1 = lv_obj_create(parent);
    style_separator(m->sep_1);
    lv_obj_set_pos(m->sep_1, SEP_1_X, SEP_Y);

    m->sep_2 = lv_obj_create(parent);
    style_separator(m->sep_2);
    lv_obj_set_pos(m->sep_2, SEP_2_X, SEP_Y);

    m->sep_3 = lv_obj_create(parent);
    style_separator(m->sep_3);
    lv_obj_set_pos(m->sep_3, SEP_3_X, SEP_Y);

    create_metric_icons(parent, m);

    willi_training_fx_create(parent, &m->fx,
                             FX_LEFT_X, FX_RIGHT_X,
                             WAVE_GREEN_BASE_Y, WAVE_GREEN_DEPTH,
                             WAVE_BLUE_BASE_Y, WAVE_BLUE_DEPTH);    

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

    set_hidden(m->sep_1, true);
    set_hidden(m->sep_2, true);
    set_hidden(m->sep_3, true);

    set_hidden(m->time_panel, true);

    set_hidden(m->icon_speed, true);
    set_hidden(m->icon_pace, true);
    set_hidden(m->icon_distance, true);
    set_hidden(m->icon_calories, true);

    willi_training_fx_hide(&m->fx);
    willi_training_fx_stop(&m->fx);
}

void willi_training_metrics_show_animated(willi_training_metrics_t *m)
{
    if(!m) return;

    set_hidden(m->sep_1, false);
    set_hidden(m->sep_2, false);
    set_hidden(m->sep_3, false);

    set_hidden(m->time_panel, false);

    set_hidden(m->icon_speed, false);
    set_hidden(m->icon_pace, false);
    set_hidden(m->icon_distance, false);
    set_hidden(m->icon_calories, false);

    willi_training_fx_show(&m->fx);
    willi_training_fx_start(&m->fx);

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

    willi_training_fx_stop(&m->fx);
    willi_training_fx_hide(&m->fx);

    set_hidden(m->sep_1, true);
    set_hidden(m->sep_2, true);
    set_hidden(m->sep_3, true);

    set_hidden(m->time_panel, true);

    set_hidden(m->icon_speed, true);
    set_hidden(m->icon_pace, true);
    set_hidden(m->icon_distance, true);
    set_hidden(m->icon_calories, true);
}