#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_refresh_init(void);
void ui_refresh_process(void);
void ui_refresh_start_timer(void);

/* tick del timer LVGL */
void ui_refresh_cb(lv_timer_t *t);

#ifdef __cplusplus
}
#endif