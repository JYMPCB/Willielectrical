#pragma once
#include "lvgl.h"
#include "willi_metric_card.h"
#include "willi_training_fx.h"
#include "willi_wifi_status.h"

typedef struct {
    willi_metric_card_t speed;
    willi_metric_card_t pace;
    willi_metric_card_t distance;
    willi_metric_card_t calories;
    willi_metric_card_t time;

    lv_obj_t *sep_1;
    lv_obj_t *sep_2;
    lv_obj_t *sep_3;

    lv_obj_t *time_panel;

    lv_obj_t *icon_speed;
    lv_obj_t *icon_pace;
    lv_obj_t *icon_distance;
    lv_obj_t *icon_calories;

    willi_training_fx_t fx;
    
} willi_training_metrics_t;

void willi_training_metrics_create(lv_obj_t *parent, willi_training_metrics_t *m);
void willi_training_metrics_hide_all(willi_training_metrics_t *m);
void willi_training_metrics_show_animated(willi_training_metrics_t *m);
void willi_training_metrics_hide_animated(willi_training_metrics_t *m);