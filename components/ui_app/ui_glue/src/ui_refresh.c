#include "ui_refresh.h"
#include "app_state.h"
#include "app_logic.h"
#include "ui_home.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static bool s_first_refresh = true;
static lv_timer_t *s_ui_refresh_timer = NULL;

static char s_last_time_str[8] = "";
static bool s_last_wifi_ok = false;

static float s_last_speed_kmh = -9999.0f;
static float s_last_target_kmh = -9999.0f;
static uint32_t s_last_elapsed_s = 0xFFFFFFFFu;
static float s_last_distance_km = -9999.0f;
static uint32_t s_last_calories = 0xFFFFFFFFu;
static train_state_t s_last_train_state = (train_state_t)(-1);
static app_mode_t s_last_mode = (app_mode_t)(-1);
static bool s_last_running_visual = false;

/* =========================================================
 * Compat helpers
 * ========================================================= */

static void safe_strcpy(char *dst, const char *src, size_t dst_size)
{
    if(dst == NULL || dst_size == 0) return;

    if(src == NULL) {
        dst[0] = '\0';
        return;
    }

    size_t i = 0;
    while(i + 1 < dst_size && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static bool localtime_safe(const time_t *timep, struct tm *out_tm)
{
    if(timep == NULL || out_tm == NULL) return false;

#if defined(_WIN32) || defined(_WIN64)
    return localtime_s(out_tm, timep) == 0;
#else
    return localtime_r(timep, out_tm) != NULL;
#endif
}

/* =========================================================
 * Time
 * ========================================================= */

static bool net_time_valid(void)
{
    time_t now = time(NULL);

#if defined(_WIN32) || defined(_WIN64)
    /* En simulación Windows usamos siempre la hora local del sistema */
    return (now > 0);
#else
    /* En embedded seguimos validando que ya exista hora real por SNTP/NTP */
    return (now > 1700000000);
#endif
}

static void format_hhmm(char *buf, size_t len)
{
    if(buf == NULL || len == 0) return;

    if(!net_time_valid()) {
        buf[0] = '\0';
        return;
    }

    time_t now = time(NULL);
    struct tm t;

    if(!localtime_safe(&now, &t)) {
        buf[0] = '\0';
        return;
    }

    strftime(buf, len, "%H:%M", &t);
}

/* =========================================================
 * Change detection
 * ========================================================= */

static bool float_changed(float a, float b, float eps)
{
    float d = a - b;
    if(d < 0.0f) d = -d;
    return d > eps;
}

/* =========================================================
 * Refresh blocks
 * ========================================================= */

static void ui_refresh_topbar(void)
{
    char time_str[8];
    format_hhmm(time_str, sizeof(time_str));

    if(strcmp(time_str, s_last_time_str) != 0) {
        safe_strcpy(s_last_time_str, time_str, sizeof(s_last_time_str));
        ui_home_set_clock_text(time_str);
    }

    if(g_app.wifi_connected != s_last_wifi_ok) {
        s_last_wifi_ok = g_app.wifi_connected;
        ui_home_set_wifi_connected(g_app.wifi_connected);
    }

    g_app.ui_req_topbar_refresh = false;
}

static void ui_refresh_home(void)
{
    bool need_sync = false;

    if(g_app.selected_mode != s_last_mode) {
        s_last_mode = g_app.selected_mode;
        need_sync = true;
    }

    if(g_app.ui_show_running_screen != s_last_running_visual) {
        s_last_running_visual = g_app.ui_show_running_screen;
        need_sync = true;
    }

    if(g_app.train_state != s_last_train_state) {
        s_last_train_state = g_app.train_state;
        need_sync = true;
    }

    if(need_sync || s_first_refresh) {
        ui_home_sync_with_app();
    }

    g_app.ui_req_home_refresh = false;
}

static void ui_refresh_running_metrics(void)
{
    bool changed = false;

    if(float_changed(g_app.current_speed_kmh, s_last_speed_kmh, 0.01f)) {
        s_last_speed_kmh = g_app.current_speed_kmh;
        changed = true;
    }

    if(float_changed(g_app.target_speed_kmh, s_last_target_kmh, 0.01f)) {
        s_last_target_kmh = g_app.target_speed_kmh;
        changed = true;
    }

    if(g_app.elapsed_time_s != s_last_elapsed_s) {
        s_last_elapsed_s = g_app.elapsed_time_s;
        changed = true;
    }

    if(float_changed(g_app.distance_km, s_last_distance_km, 0.001f)) {
        s_last_distance_km = g_app.distance_km;
        changed = true;
    }

    if(g_app.calories != s_last_calories) {
        s_last_calories = g_app.calories;
        changed = true;
    }

    if(changed || s_first_refresh) {
        ui_home_set_metrics(
            g_app.current_speed_kmh,
            g_app.target_speed_kmh,
            g_app.elapsed_time_s,
            g_app.distance_km,
            g_app.calories
        );
    }

    g_app.ui_req_running_refresh = false;
}

static void ui_refresh_screen_change(void)
{
    ui_home_sync_with_app();
    g_app.ui_req_screen_change = false;
}

/* =========================================================
 * Public API
 * ========================================================= */

void ui_refresh_init(void)
{
    s_first_refresh = true;

    s_last_time_str[0] = '\0';
    s_last_wifi_ok = false;

    s_last_speed_kmh = -9999.0f;
    s_last_target_kmh = -9999.0f;
    s_last_elapsed_s = 0xFFFFFFFFu;
    s_last_distance_km = -9999.0f;
    s_last_calories = 0xFFFFFFFFu;
    s_last_train_state = (train_state_t)(-1);
    s_last_mode = (app_mode_t)(-1);
    s_last_running_visual = false;
}

void ui_refresh_process(void)
{
    if(s_first_refresh) {
        g_app.ui_req_topbar_refresh = true;
        g_app.ui_req_home_refresh = true;
        g_app.ui_req_running_refresh = true;
        g_app.ui_req_screen_change = true;
    }

    if(g_app.ui_req_screen_change) {
        ui_refresh_screen_change();
    }

    if(g_app.ui_req_topbar_refresh) {
        ui_refresh_topbar();
    }

    if(g_app.ui_req_home_refresh) {
        ui_refresh_home();
    }

    if(g_app.ui_req_running_refresh) {
        ui_refresh_running_metrics();
    }

    s_first_refresh = false;
}

void ui_refresh_cb(lv_timer_t *t)
{
    (void)t;
    //printf("ui_refresh_cb tick\n");
    app_logic_process();
    ui_refresh_process();
}

void ui_refresh_start_timer(void)
{
    printf("ui_refresh_start_timer called\n");
    if(s_ui_refresh_timer == NULL) {
        s_ui_refresh_timer = lv_timer_create(ui_refresh_cb, 100, NULL);
    } else {
        lv_timer_set_period(s_ui_refresh_timer, 100);
        lv_timer_resume(s_ui_refresh_timer);
    }
}

void ui_refresh_stop_timer(void)
{
    if(s_ui_refresh_timer != NULL) {
        lv_timer_pause(s_ui_refresh_timer);
    }
}