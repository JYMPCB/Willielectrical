#include "app_control.h"

bool app_can_start(void)
{
    printf("START clicked\n");
    return (g_app.train_state == TRAIN_STATE_IDLE);
    if(g_app.error_active) return false;
    if(!g_app.safety_key_ok) return false;
    if(!g_app.vfd_ready) return false;

    return (g_app.train_state == TRAIN_STATE_IDLE);
}

bool app_can_stop(void)
{
    return (g_app.train_state == TRAIN_STATE_RUNNING ||
            g_app.train_state == TRAIN_STATE_STARTING);
}

void app_request_start(app_input_src_t src)
{
    g_app.last_input_src = src;
    g_app.start_req = true;
}

void app_request_stop(app_input_src_t src)
{
    g_app.last_input_src = src;
    g_app.stop_req = true;
}

void app_request_speed_up(app_input_src_t src)
{
    g_app.last_input_src = src;
    g_app.speed_up_req = true;
}

void app_request_speed_down(app_input_src_t src)
{
    g_app.last_input_src = src;
    g_app.speed_down_req = true;
}

void app_request_mode_next(app_input_src_t src)
{
    g_app.last_input_src = src;
    g_app.mode_req = true;
}