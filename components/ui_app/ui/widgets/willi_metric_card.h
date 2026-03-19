#pragma once
#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *label_title;
    lv_obj_t *label_value;
    lv_obj_t *label_unit;
    lv_coord_t final_x;
    lv_coord_t final_y;
} willi_metric_card_t;

void willi_metric_card_create(willi_metric_card_t *card, lv_obj_t *parent);

void willi_metric_card_set_title(willi_metric_card_t *card, const char *txt);
void willi_metric_card_set_value(willi_metric_card_t *card, const char *txt);
void willi_metric_card_set_unit(willi_metric_card_t *card, const char *txt);

void willi_metric_card_set_title_color(willi_metric_card_t *card, lv_color_t color);
void willi_metric_card_set_value_color(willi_metric_card_t *card, lv_color_t color);
void willi_metric_card_set_unit_color(willi_metric_card_t *card, lv_color_t color);

void willi_metric_card_set_pos(willi_metric_card_t *card, lv_coord_t x, lv_coord_t y);
void willi_metric_card_set_size(willi_metric_card_t *card, lv_coord_t w, lv_coord_t h);

void willi_metric_card_hide(willi_metric_card_t *card);
void willi_metric_card_show(willi_metric_card_t *card);

void willi_metric_card_prepare_in_anim(willi_metric_card_t *card, lv_coord_t y_offset);
void willi_metric_card_animate_in(willi_metric_card_t *card, uint32_t delay_ms);

void willi_metric_card_prepare_out_anim(willi_metric_card_t *card, lv_coord_t y_offset);
void willi_metric_card_animate_out(willi_metric_card_t *card, uint32_t delay_ms);