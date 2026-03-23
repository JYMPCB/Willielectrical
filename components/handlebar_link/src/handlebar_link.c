#include "handlebar_link.h"
#include "bus_master.h"
#include "vfd_link.h"
#include "app_control.h"
#include "pins_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "handlebar_link";

#define TREADMILL_SPEED_STEP_X100      50
#define TREADMILL_SPEED_MIN_X100       100
#define TREADMILL_SPEED_MAX_X100       2200
#define HANDLEBAR_POLL_MS              20
#define CMD_TRACE_TIMEOUT_US           3000000LL

static int32_t s_target_speed_x100 = TREADMILL_SPEED_MIN_X100;

typedef struct {
    bool active;
    uint8_t ev_type;
    uint8_t expected_run_fb;
    int64_t t_event_us;
    int64_t t_ack_us;
} cmd_trace_t;

static cmd_trace_t s_cmd_trace;

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static const char *event_name(uint8_t ev_type)
{
    switch (ev_type) {
        case HANDLE_EV_START: return "START";
        case HANDLE_EV_STOP: return "STOP";
        case HANDLE_EV_MODE: return "MODE";
        case HANDLE_EV_UP: return "UP";
        case HANDLE_EV_DOWN: return "DOWN";
        default: return "UNKNOWN";
    }
}

static void trace_cmd_start(uint8_t ev_type, uint8_t expected_run_fb, int64_t t_event_us, int64_t t_ack_us)
{
    s_cmd_trace.active = true;
    s_cmd_trace.ev_type = ev_type;
    s_cmd_trace.expected_run_fb = expected_run_fb;
    s_cmd_trace.t_event_us = t_event_us;
    s_cmd_trace.t_ack_us = t_ack_us;
}

static void trace_cmd_feedback_if_ready(void)
{
    if (!s_cmd_trace.active) {
        return;
    }

    int64_t now_us = esp_timer_get_time();
    if ((now_us - s_cmd_trace.t_event_us) > CMD_TRACE_TIMEOUT_US) {
        ESP_LOGW(TAG,
                 "lat %s timeout waiting run_fb=%u (event->ack=%.1f ms)",
                 event_name(s_cmd_trace.ev_type),
                 (unsigned)s_cmd_trace.expected_run_fb,
                 (double)(s_cmd_trace.t_ack_us - s_cmd_trace.t_event_us) / 1000.0);
        s_cmd_trace.active = false;
        return;
    }

    vfd_status_t vfd_st;
    if (!vfd_link_get_status(&vfd_st)) {
        return;
    }

    if (vfd_st.run_fb != s_cmd_trace.expected_run_fb) {
        return;
    }

    double t_event_ack_ms = (double)(s_cmd_trace.t_ack_us - s_cmd_trace.t_event_us) / 1000.0;
    double t_ack_feedback_ms = (double)(now_us - s_cmd_trace.t_ack_us) / 1000.0;
    double t_total_ms = (double)(now_us - s_cmd_trace.t_event_us) / 1000.0;

    ESP_LOGI(TAG,
             "lat %s ok: event->ack=%.1f ms ack->run_fb=%.1f ms total=%.1f ms",
             event_name(s_cmd_trace.ev_type),
             t_event_ack_ms,
             t_ack_feedback_ms,
             t_total_ms);
    s_cmd_trace.active = false;
}

static void handle_event_drive_treadmill(const bus_event_item_t *ev)
{
    if (!ev) {
        return;
    }

    int64_t t_event_us = esp_timer_get_time();

    switch (ev->type) {
        case HANDLE_EV_START:
            app_request_start(APP_INPUT_HANDLEBAR);
            ESP_LOGI(TAG, "req START forwarded to app_core");
            break;

        case HANDLE_EV_STOP:
            app_request_stop(APP_INPUT_HANDLEBAR);
            ESP_LOGI(TAG, "req STOP forwarded to app_core");
            break;

        case HANDLE_EV_UP:
            s_target_speed_x100 = clamp_i32(s_target_speed_x100 + TREADMILL_SPEED_STEP_X100,
                                            TREADMILL_SPEED_MIN_X100,
                                            TREADMILL_SPEED_MAX_X100);
            app_request_speed_up(APP_INPUT_HANDLEBAR);
            ESP_LOGI(TAG, "req SPEED UP forwarded to app_core (target=%.2f km/h)",
                     (double)s_target_speed_x100 / 100.0);
            break;

        case HANDLE_EV_DOWN:
            s_target_speed_x100 = clamp_i32(s_target_speed_x100 - TREADMILL_SPEED_STEP_X100,
                                            TREADMILL_SPEED_MIN_X100,
                                            TREADMILL_SPEED_MAX_X100);
            app_request_speed_down(APP_INPUT_HANDLEBAR);
            ESP_LOGI(TAG, "req SPEED DOWN forwarded to app_core (target=%.2f km/h)",
                     (double)s_target_speed_x100 / 100.0);
            break;

        case HANDLE_EV_MODE:
            app_request_mode_next(APP_INPUT_HANDLEBAR);
            ESP_LOGI(TAG, "req MODE forwarded to app_core");
            break;

        default:
            break;
    }
}

typedef struct {
    bool inited;
    bool online;
    bus_info_payload_t info;
    handlebar_status_t status;
    uint32_t ok_count;
    uint32_t fail_count;
} handlebar_link_ctx_t;

static handlebar_link_ctx_t s_ctx;

static bool request_simple(uint8_t cmd, bus_packet_t *rsp, int timeout_ms)
{
    return bus_master_request(BUS_ADDR_HANDLEBAR, cmd, NULL, 0, rsp, timeout_ms);
}

bool handlebar_link_init(void)
{
    memset(&s_ctx, 0, sizeof(s_ctx));

    bus_master_config_t cfg = {
        .uart_num      = HMI_UART_NUM,
        .tx_pin        = PIN_RS485_TX,
        .rx_pin        = PIN_RS485_RX,
        .baudrate      = HMI_UART_BAUD,
        .self_addr     = BUS_ADDR_HMI_MASTER,
        .seq_counter   = 0,
        .rx_timeout_ms = 50
    };

    bool ok = bus_master_init(&cfg);
    s_ctx.inited = ok;
    return ok;
}

bool handlebar_link_ping(void)
{
    bus_packet_t rsp;
    bool ok = request_simple(BUS_CMD_PING, &rsp, 50);
    if (!ok || rsp.cmd != BUS_CMD_PONG) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

bool handlebar_link_read_info(void)
{
    bus_packet_t rsp;
    bool ok = request_simple(BUS_CMD_GET_INFO, &rsp, 50);
    if (!ok || rsp.cmd != BUS_CMD_INFO_REPLY || rsp.len < sizeof(bus_info_payload_t)) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    memcpy(&s_ctx.info, rsp.data, sizeof(bus_info_payload_t));
    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

bool handlebar_link_read_status(void)
{
    bus_packet_t rsp;
    bool ok = request_simple(BUS_CMD_GET_STATUS, &rsp, 50);
    if (!ok || rsp.cmd != BUS_CMD_STATUS_REPLY || rsp.len < sizeof(handlebar_status_t)) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    memcpy(&s_ctx.status, rsp.data, sizeof(handlebar_status_t));
    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

int handlebar_link_read_events(bus_event_item_t *items, int max_items)
{
    if (!items || max_items <= 0) {
        return 0;
    }

    bus_packet_t rsp;
    bool ok = request_simple(BUS_CMD_GET_EVENTS, &rsp, 50);
    if (!ok || rsp.cmd != BUS_CMD_EVENTS_REPLY || rsp.len < 1) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return 0;
    }

    uint8_t count = rsp.data[0];
    size_t expected = 1 + ((size_t)count * sizeof(bus_event_item_t));
    if (rsp.len < expected) {
        s_ctx.fail_count++;
        return 0;
    }

    int ncopy = (count < (uint8_t)max_items) ? count : max_items;
    if (ncopy > 0) {
        memcpy(items, &rsp.data[1], ncopy * sizeof(bus_event_item_t));
    }

    s_ctx.online = true;
    s_ctx.ok_count++;
    return ncopy;
}

bool handlebar_link_set_mode(handlebar_mode_t mode)
{
    handlebar_set_mode_t req = {
        .mode = (uint8_t)mode
    };

    bus_packet_t rsp;
    bool ok = bus_master_request(BUS_ADDR_HANDLEBAR,
                                 BUS_CMD_SET_MODE,
                                 (const uint8_t *)&req,
                                 sizeof(req),
                                 &rsp,
                                 50);

    if (!ok || rsp.cmd != BUS_CMD_ACK) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    if (rsp.len < sizeof(bus_ack_payload_t)) {
        s_ctx.fail_count++;
        return false;
    }

    bus_ack_payload_t ack;
    memcpy(&ack, rsp.data, sizeof(ack));
    if (ack.ack_cmd != BUS_CMD_SET_MODE) {
        s_ctx.fail_count++;
        return false;
    }

    s_ctx.status.mode = (uint8_t)mode;
    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

bool handlebar_link_clear_events(void)
{
    bus_packet_t rsp;
    bool ok = request_simple(BUS_CMD_CLEAR_EVENTS, &rsp, 50);

    if (!ok || rsp.cmd != BUS_CMD_ACK || rsp.len < sizeof(bus_ack_payload_t)) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    bus_ack_payload_t ack;
    memcpy(&ack, rsp.data, sizeof(ack));
    if (ack.ack_cmd != BUS_CMD_CLEAR_EVENTS) {
        s_ctx.fail_count++;
        return false;
    }

    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

bool handlebar_link_is_online(void)
{
    return s_ctx.online;
}

bool handlebar_link_get_info(bus_info_payload_t *out_info)
{
    if (!out_info) return false;
    *out_info = s_ctx.info;
    return true;
}

bool handlebar_link_get_status(handlebar_status_t *out_status)
{
    if (!out_status) return false;
    *out_status = s_ctx.status;
    return true;
}

uint32_t handlebar_link_get_ok_count(void)
{
    return s_ctx.ok_count;
}

uint32_t handlebar_link_get_fail_count(void)
{
    return s_ctx.fail_count;
}

void handlebar_link_task(void *arg)
{
    (void)arg;

    bool info_read = false;
    bool was_offline = false;
    TickType_t last_offline_log = 0;

    while (1) {
        if (!handlebar_link_ping()) {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_offline_log) >= pdMS_TO_TICKS(3000)) {
                last_offline_log = now;
                ESP_LOGW(TAG, "handlebar offline (ok=%lu fail=%lu)",
                         (unsigned long)s_ctx.ok_count,
                         (unsigned long)s_ctx.fail_count);
            }
            was_offline = true;
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (was_offline) {
            ESP_LOGI(TAG, "handlebar online again");
            was_offline = false;
        }

        if (!info_read) {
            if (handlebar_link_read_info()) {
                info_read = true;
                ESP_LOGI(TAG, "handlebar info ok: name=%s fw=%u.%u build=%lu",
                         s_ctx.info.name,
                         s_ctx.info.fw_major,
                         s_ctx.info.fw_minor,
                         (unsigned long)s_ctx.info.fw_build);
            }
        }

        if (handlebar_link_read_status()) {
            if (s_ctx.status.events_pending > 0) {
                bus_event_item_t events[15];
                int n = handlebar_link_read_events(events, 15);

                int stop_idx = -1;
                for (int i = 0; i < n; i++) {
                    if (events[i].type == HANDLE_EV_STOP) {
                        stop_idx = i;
                        break;
                    }
                }

                if (stop_idx >= 0) {
                    ESP_LOGI(TAG, "ev type=%u param=%u t=%u",
                             events[stop_idx].type,
                             events[stop_idx].param,
                             events[stop_idx].time_ms);
                    handle_event_drive_treadmill(&events[stop_idx]);
                }

                for (int i = 0; i < n; i++) {
                    if (i == stop_idx) {
                        continue;
                    }

                    ESP_LOGI(TAG, "ev type=%u param=%u t=%u",
                             events[i].type,
                             events[i].param,
                             events[i].time_ms);

                    handle_event_drive_treadmill(&events[i]);

                    // Acá luego integrás con tu lógica real del HMI.
                    // Ejemplo:
                    // START -> iniciar entrenamiento
                    // STOP  -> parar
                    // MODE  -> alternar modo y luego SET_MODE()
                    // UP    -> subir velocidad o ritmo
                    // DOWN  -> bajar velocidad o ritmo

                    if (events[i].type == HANDLE_EV_MODE) {
                        handlebar_mode_t new_mode =
                            (s_ctx.status.mode == HANDLE_MODE_SPEED) ? HANDLE_MODE_PACE
                                                                     : HANDLE_MODE_SPEED;

                        handlebar_link_set_mode(new_mode);
                    }
                }
            }
        }

        trace_cmd_feedback_if_ready();

        vTaskDelay(pdMS_TO_TICKS(HANDLEBAR_POLL_MS));
    }
}