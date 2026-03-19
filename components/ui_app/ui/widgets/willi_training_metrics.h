#pragma once
#include "lvgl.h"
#include "willi_metric_card.h"

#define WILLI_CURVE_PTS 72

#if defined(LVGL_VERSION_MAJOR) && (LVGL_VERSION_MAJOR >= 9)
typedef lv_point_precise_t willi_curve_point_t;
#else
typedef lv_point_t willi_curve_point_t;
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

    lv_obj_t *curve_1;
    lv_obj_t *curve_2;
    lv_obj_t *curve_3;
    lv_obj_t *curve_1_glow;

    lv_obj_t *travel_dot_1;
    lv_obj_t *travel_dot_2;
    lv_obj_t *travel_dot_1_halo;
    lv_obj_t *travel_dot_2_halo;

    lv_obj_t *wifi_label;

    willi_curve_point_t curve1_pts[WILLI_CURVE_PTS];
    willi_curve_point_t curve2_pts[WILLI_CURVE_PTS];
    willi_curve_point_t curve3_pts[WILLI_CURVE_PTS];

    lv_timer_t *curve_timer;
    lv_timer_t *wifi_timer;

    float dot_pos_1;
    float dot_pos_2;
    uint8_t wifi_phase;
} willi_training_metrics_t;

void willi_training_metrics_create(lv_obj_t *parent, willi_training_metrics_t *m);
void willi_training_metrics_hide_all(willi_training_metrics_t *m);
void willi_training_metrics_show_animated(willi_training_metrics_t *m);
void willi_training_metrics_hide_animated(willi_training_metrics_t *m);