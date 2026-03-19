#pragma once
#include "lvgl.h"
#include "willi_metric_card.h"

#define WILLI_CURVE_PTS 48
#define WILLI_CURVE_SEGS (WILLI_CURVE_PTS - 1)

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

    lv_obj_t *curve_1_segs[WILLI_CURVE_SEGS];
    lv_obj_t *curve_2_segs[WILLI_CURVE_SEGS];
    lv_obj_t *curve_3_segs[WILLI_CURVE_SEGS];

    lv_obj_t *travel_dot_1;
    lv_obj_t *travel_dot_2;

    lv_point_t curve1_pts[WILLI_CURVE_PTS];
    lv_point_t curve2_pts[WILLI_CURVE_PTS];
    lv_point_t curve3_pts[WILLI_CURVE_PTS];

    lv_timer_t *curve_timer;
    uint16_t dot_idx_1;
    uint16_t dot_idx_2;
} willi_training_metrics_t;

void willi_training_metrics_create(lv_obj_t *parent, willi_training_metrics_t *m);
void willi_training_metrics_hide_all(willi_training_metrics_t *m);
void willi_training_metrics_show_animated(willi_training_metrics_t *m);
void willi_training_metrics_hide_animated(willi_training_metrics_t *m);