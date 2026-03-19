#pragma once
#include "lvgl.h"
#include "willi_metric_card.h"

typedef struct {
    willi_metric_card_t speed;
    willi_metric_card_t pace;
    willi_metric_card_t distance;
    willi_metric_card_t time;
    willi_metric_card_t calories;
} willi_training_metrics_t;

void willi_training_metrics_create(lv_obj_t *parent, willi_training_metrics_t *m);
void willi_training_metrics_hide_all(willi_training_metrics_t *m);
void willi_training_metrics_show_animated(willi_training_metrics_t *m);
void willi_training_metrics_hide_animated(willi_training_metrics_t *m);