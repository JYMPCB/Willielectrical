#pragma once
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    lv_obj_t *label;
    lv_timer_t *timer;
    uint8_t phase;
} willi_wifi_status_t;

void willi_wifi_status_create(lv_obj_t *parent, willi_wifi_status_t *w, lv_coord_t x, lv_coord_t y);
void willi_wifi_status_show(willi_wifi_status_t *w);
void willi_wifi_status_hide(willi_wifi_status_t *w);
void willi_wifi_status_start(willi_wifi_status_t *w);
void willi_wifi_status_stop(willi_wifi_status_t *w);