#pragma once
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *ui_home_create(void);
void ui_home_sync_with_app(void);

void ui_home_set_clock_text(const char *txt);
void ui_home_set_wifi_connected(bool connected);
void ui_home_set_metrics(float current_speed_kmh,
                         float target_speed_kmh,
                         uint32_t elapsed_time_s,
                         float distance_km,
                         uint32_t calories);

#ifdef __cplusplus
}
#endif