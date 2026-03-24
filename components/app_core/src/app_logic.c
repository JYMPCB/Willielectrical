#include "app_logic.h"
#include "app_state.h"
#include "app_control.h"
#include "vfd_link.h"
#include "audio_mgr.h"
#include "lvgl.h"
#include <stdio.h>

#define APP_START_UI_DELAY_MS   600U

#define APP_MIN_SPEED_KMH       0.5f
#define APP_MAX_SPEED_KMH       18.0f

static int32_t kmh_to_x100(float kmh)
{
    if(kmh < 0.0f) kmh = 0.0f;
    return (int32_t)(kmh * 100.0f + 0.5f);
}

static float clampf_local(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void app_prepare_session(void)
{
    g_app.elapsed_time_s = 0;
    g_app.distance_km = 0.0f;
    g_app.calories = 0;
    g_app.current_speed_kmh = 0.0f;

    g_app.start_hw_sent = false;
    g_app.start_sequence_ms = 0;

    switch(g_app.selected_mode) {
        case APP_MODE_PROGRAM:
            g_app.target_speed_kmh = 4.0f;
            break;
        case APP_MODE_FREE:
            g_app.target_speed_kmh = 3.0f;
            break;
        case APP_MODE_INTERVAL:
            g_app.target_speed_kmh = 5.0f;
            break;
        default:
            g_app.target_speed_kmh = 3.0f;
            break;
    }
}

static void app_cycle_mode(void)
{
    if(g_app.train_state != TRAIN_STATE_IDLE) return;

    switch(g_app.selected_mode) {
        case APP_MODE_PROGRAM:
            g_app.selected_mode = APP_MODE_FREE;
            break;
        case APP_MODE_FREE:
            g_app.selected_mode = APP_MODE_INTERVAL;
            break;
        case APP_MODE_INTERVAL:
        default:
            g_app.selected_mode = APP_MODE_PROGRAM;
            break;
    }

    g_app.ui_req_home_refresh = true;
}

static void app_process_start(void)
{
    if(!g_app.start_req) return;
    g_app.start_req = false;

    printf("app_process_start: error=%d safety=%d vfd_ready=%d state=%d\n",
           g_app.error_active,
           g_app.safety_key_ok,
           g_app.vfd_ready,
           g_app.train_state);

    if(!app_can_start()) {
        printf("app_process_start: START BLOQUEADO\n");
        return;
    }

    app_prepare_session();

    g_app.train_state = TRAIN_STATE_STARTING;
    g_app.ui_show_running_screen = true;
    g_app.start_sequence_ms = lv_tick_get();
    g_app.start_hw_sent = false;
    audio_play(AUDIO_EVT_START);

    printf("app_process_start: STARTING OK\n");

    g_app.ui_req_home_refresh = true;
    g_app.ui_req_running_refresh = true;
    g_app.ui_req_screen_change = true;
}

static void app_process_stop(void)
{
    if(!g_app.stop_req) return;
    g_app.stop_req = false;

    if(!app_can_stop()) {
        return;
    }

    g_app.target_speed_kmh = 0.0f;
    g_app.start_hw_sent = false;
    g_app.train_state = TRAIN_STATE_STOPPING;
    audio_play(AUDIO_EVT_STOP);

    g_app.ui_req_running_refresh = true;
}

static void app_process_speed_up(void)
{
    if(!g_app.speed_up_req) return;
    g_app.speed_up_req = false;

    if(g_app.train_state != TRAIN_STATE_RUNNING) return;

    g_app.target_speed_kmh += 0.5f;
    g_app.target_speed_kmh = clampf_local(g_app.target_speed_kmh, APP_MIN_SPEED_KMH, APP_MAX_SPEED_KMH);

    if(!vfd_link_set_speed_kmh_x100(kmh_to_x100(g_app.target_speed_kmh))) {
        printf("app_process_speed_up: vfd set speed failed\n");
    }

    audio_play(AUDIO_EVT_SPEED_UP);

    g_app.ui_req_running_refresh = true;
}

static void app_process_speed_down(void)
{
    if(!g_app.speed_down_req) return;
    g_app.speed_down_req = false;

    if(g_app.train_state != TRAIN_STATE_RUNNING) return;

    g_app.target_speed_kmh -= 0.5f;
    g_app.target_speed_kmh = clampf_local(g_app.target_speed_kmh, APP_MIN_SPEED_KMH, APP_MAX_SPEED_KMH);

    if(!vfd_link_set_speed_kmh_x100(kmh_to_x100(g_app.target_speed_kmh))) {
        printf("app_process_speed_down: vfd set speed failed\n");
    }

    audio_play(AUDIO_EVT_SPEED_DOWN);

    g_app.ui_req_running_refresh = true;
}

static void app_process_pace_up(void)
{
    if(!g_app.pace_up_req) return;
    g_app.pace_up_req = false;

    if(g_app.train_state != TRAIN_STATE_RUNNING) return;

    // Ritmo +/- debe sentirse equivalente a velocidad +/- en respuesta.
    g_app.target_speed_kmh += 0.5f;
    g_app.target_speed_kmh = clampf_local(g_app.target_speed_kmh, APP_MIN_SPEED_KMH, APP_MAX_SPEED_KMH);

    if(!vfd_link_set_speed_kmh_x100(kmh_to_x100(g_app.target_speed_kmh))) {
        printf("app_process_pace_up: vfd set speed failed\n");
    }

    audio_play(AUDIO_EVT_PACE_UP);

    g_app.ui_req_running_refresh = true;
}

static void app_process_pace_down(void)
{
    if(!g_app.pace_down_req) return;
    g_app.pace_down_req = false;

    if(g_app.train_state != TRAIN_STATE_RUNNING) return;

    // Ritmo +/- debe sentirse equivalente a velocidad +/- en respuesta.
    g_app.target_speed_kmh -= 0.5f;
    g_app.target_speed_kmh = clampf_local(g_app.target_speed_kmh, APP_MIN_SPEED_KMH, APP_MAX_SPEED_KMH);

    if(!vfd_link_set_speed_kmh_x100(kmh_to_x100(g_app.target_speed_kmh))) {
        printf("app_process_pace_down: vfd set speed failed\n");
    }

    audio_play(AUDIO_EVT_PACE_DOWN);

    g_app.ui_req_running_refresh = true;
}

static void app_process_mode(void)
{
    if(!g_app.mode_req) return;
    g_app.mode_req = false;

    audio_play(AUDIO_EVT_MODE);
    app_cycle_mode();
}

static void app_process_exit_running_screen(void)
{
    if(!g_app.exit_running_screen_req) return;

    if(g_app.train_state == TRAIN_STATE_RUNNING ||
       g_app.train_state == TRAIN_STATE_STARTING) {
        g_app.stop_req = true;
        return;
    }

    if(g_app.train_state == TRAIN_STATE_STOPPING) {
        return;
    }

    if(g_app.ui_show_running_screen) {
        g_app.ui_show_running_screen = false;
        g_app.ui_req_home_refresh = true;
        g_app.ui_req_screen_change = true;
    }

    g_app.exit_running_screen_req = false;
}

static void app_update_state_machine(void)
{
    switch(g_app.train_state) {
        case TRAIN_STATE_STARTING:
            if(!g_app.start_hw_sent &&
               lv_tick_elaps(g_app.start_sequence_ms) >= APP_START_UI_DELAY_MS) {

                g_app.start_hw_sent = true;
                bool hw_ok = true;
                if(!vfd_link_set_speed_kmh_x100(kmh_to_x100(g_app.target_speed_kmh))) {
                    printf("app_update_state_machine: vfd set speed failed\n");
                    hw_ok = false;
                }
                if(!vfd_link_start()) {
                    printf("app_update_state_machine: vfd start failed\n");
                    hw_ok = false;
                }

                if(!hw_ok) {
                    printf("app_update_state_machine: fallback to simulated RUNNING\n");
                }

                g_app.current_speed_kmh = g_app.target_speed_kmh;
                g_app.train_state = TRAIN_STATE_RUNNING;
                g_app.ui_req_running_refresh = true;
                g_app.ui_req_home_refresh = true;
            }
            break;

        case TRAIN_STATE_RUNNING:
            /* El valor real después vendrá desde sensor/VFD */
            break;

        case TRAIN_STATE_STOPPING:
            (void)vfd_link_set_speed_kmh_x100(0);
            if(!vfd_link_stop()) {
                printf("app_update_state_machine: vfd stop failed\n");
            }
            g_app.current_speed_kmh = 0.0f;
            g_app.train_state = TRAIN_STATE_IDLE;
            g_app.start_hw_sent = false;

            if(g_app.exit_running_screen_req) {
                g_app.ui_show_running_screen = false;
                g_app.exit_running_screen_req = false;
                g_app.ui_req_screen_change = true;
            }

            g_app.ui_req_home_refresh = true;
            g_app.ui_req_running_refresh = true;
            break;

        case TRAIN_STATE_IDLE:
        case TRAIN_STATE_ERROR:
        default:
            break;
    }
}

void app_logic_init(void)
{
}

void app_logic_process(void)
{
    //printf("app_logic_process running start_req=%d\n", g_app.start_req);
    app_process_start();
    app_process_stop();
    app_process_speed_up();
    app_process_speed_down();
    app_process_pace_up();
    app_process_pace_down();
    app_process_mode();
    app_process_exit_running_screen();

    app_update_state_machine();
}

void app_logic_tick_1s(void)
{
    if(g_app.train_state == TRAIN_STATE_RUNNING) {
        g_app.elapsed_time_s++;

        g_app.distance_km += (g_app.current_speed_kmh / 3600.0f);
        g_app.calories += 1;

        g_app.ui_req_running_refresh = true;
    }
}