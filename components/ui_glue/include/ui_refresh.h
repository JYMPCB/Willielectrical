#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_refresh_cb(lv_timer_t *t);
void ui_refresh_start_timer(void);

#ifdef __cplusplus
}
#endif
