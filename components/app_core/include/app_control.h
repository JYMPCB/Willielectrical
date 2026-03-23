#pragma once

#include "app_state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_request_start(app_input_src_t src);
void app_request_stop(app_input_src_t src);
void app_request_exit_running_screen(app_input_src_t src);
void app_request_speed_up(app_input_src_t src);
void app_request_speed_down(app_input_src_t src);
void app_request_mode_next(app_input_src_t src);

bool app_can_start(void);
bool app_can_stop(void);

#ifdef __cplusplus
}
#endif