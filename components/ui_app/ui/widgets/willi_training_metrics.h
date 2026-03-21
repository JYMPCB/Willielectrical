#pragma once

#include "lvgl.h"
#include "willi_metric_card.h"
#include "willi_training_fx.h"

#ifdef __cplusplus
extern "C" {
#endif

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

void willi_training_metrics_set_speed(willi_training_metrics_t *m, const char *txt);
void willi_training_metrics_set_pace(willi_training_metrics_t *m, const char *txt);
void willi_training_metrics_set_distance(willi_training_metrics_t *m, const char *txt);
void willi_training_metrics_set_calories(willi_training_metrics_t *m, const char *txt);
void willi_training_metrics_set_time(willi_training_metrics_t *m, const char *txt);

#ifdef __cplusplus
}
#endif