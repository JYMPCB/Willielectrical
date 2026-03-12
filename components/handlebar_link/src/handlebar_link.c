#include "handlebar_link.h"
#include "bus_master.h"
#include "pins_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "handlebar_link";

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

    while (1) {
        if (!handlebar_link_ping()) {
            ESP_LOGW(TAG, "handlebar offline");
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
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

                for (int i = 0; i < n; i++) {
                    ESP_LOGI(TAG, "ev[%d]: type=%u param=%u t=%u",
                             i,
                             events[i].type,
                             events[i].param,
                             events[i].time_ms);

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

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}