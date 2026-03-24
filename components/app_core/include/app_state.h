#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_MODE_PROGRAM = 0,
    APP_MODE_FREE,
    APP_MODE_INTERVAL
} app_mode_t;

typedef enum {
    TRAIN_STATE_IDLE = 0,
    TRAIN_STATE_STARTING,
    TRAIN_STATE_RUNNING,
    TRAIN_STATE_STOPPING,
    TRAIN_STATE_ERROR
} train_state_t;

typedef enum {
    APP_INPUT_NONE = 0,
    APP_INPUT_UI,
    APP_INPUT_HANDLEBAR,
    APP_INPUT_REMOTE
} app_input_src_t;

typedef struct {
    app_mode_t selected_mode;
    train_state_t train_state;
    app_input_src_t last_input_src;

    bool safety_key_ok;
    bool error_active;
    bool vfd_ready;
    bool wifi_connected;
    int8_t wifi_rssi_dbm;
    uint8_t wifi_signal_level;

    bool start_req;
    bool stop_req;
    bool exit_running_screen_req;
    bool speed_up_req;
    bool speed_down_req;
    bool pace_up_req;
    bool pace_down_req;
    bool mode_req;

    float current_speed_kmh;
    float target_speed_kmh;

    uint32_t elapsed_time_s;
    float distance_km;
    uint32_t calories;

    bool ui_req_topbar_refresh;
    bool ui_req_home_refresh;
    bool ui_req_running_refresh;
    bool ui_req_screen_change;

    bool ui_show_running_screen;

    /* Secuencia visual/lógica de arranque */
    uint32_t start_sequence_ms;
    bool start_hw_sent;

} app_state_t;

extern app_state_t g_app;

void app_state_init(void);

#ifdef __cplusplus
}
#endif