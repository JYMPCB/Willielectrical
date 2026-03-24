#include "app_state.h"

app_state_t g_app;

void app_state_init(void)
{
    g_app.selected_mode = APP_MODE_FREE;
    g_app.train_state = TRAIN_STATE_IDLE;
    g_app.last_input_src = APP_INPUT_NONE;

    g_app.safety_key_ok = true;
    g_app.error_active = false;
    g_app.vfd_ready = true;
    g_app.wifi_connected = false;
    g_app.wifi_rssi_dbm = -127;
    g_app.wifi_signal_level = 0;

    g_app.start_req = false;
    g_app.stop_req = false;
    g_app.exit_running_screen_req = false;
    g_app.speed_up_req = false;
    g_app.speed_down_req = false;
    g_app.pace_up_req = false;
    g_app.pace_down_req = false;
    g_app.mode_req = false;

    g_app.current_speed_kmh = 0.0f;
    g_app.target_speed_kmh = 0.0f;

    g_app.elapsed_time_s = 0;
    g_app.distance_km = 0.0f;
    g_app.calories = 0;

    g_app.ui_req_topbar_refresh = true;
    g_app.ui_req_home_refresh = true;
    g_app.ui_req_running_refresh = true;
    g_app.ui_req_screen_change = true;

    g_app.ui_show_running_screen = false;

    g_app.start_sequence_ms = 0;
    g_app.start_hw_sent = false;
}