#pragma once
#include "lvgl.h"
#include <stdbool.h>

#define WILLI_WAVE_PTS    64
#define WILLI_TRAIL_COUNT 8

typedef struct {
    lv_obj_t *travel_dot_green;
    lv_obj_t *travel_dot_blue;
    lv_obj_t *travel_dot_green_halo;
    lv_obj_t *travel_dot_blue_halo;

    lv_obj_t *trail_green[WILLI_TRAIL_COUNT];
    lv_obj_t *trail_blue[WILLI_TRAIL_COUNT];

    lv_point_t wave_green_pts[WILLI_WAVE_PTS];
    lv_point_t wave_blue_pts[WILLI_WAVE_PTS];

    lv_timer_t *timer;

    float travel_pos_green;
    float travel_pos_blue;

    lv_coord_t x_left;
    lv_coord_t x_right;
    lv_coord_t green_base_y;
    lv_coord_t green_depth;
    lv_coord_t blue_base_y;
    lv_coord_t blue_depth;
} willi_training_fx_t;

void willi_training_fx_create(lv_obj_t *parent,
                              willi_training_fx_t *fx,
                              lv_coord_t x_left,
                              lv_coord_t x_right,
                              lv_coord_t green_base_y,
                              lv_coord_t green_depth,
                              lv_coord_t blue_base_y,
                              lv_coord_t blue_depth);

void willi_training_fx_show(willi_training_fx_t *fx);
void willi_training_fx_hide(willi_training_fx_t *fx);
void willi_training_fx_start(willi_training_fx_t *fx);
void willi_training_fx_stop(willi_training_fx_t *fx);