#include "vfd_link.h"
#include "bus_master.h"
#include "pins_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "vfd_link";

#define VFD_PING_PERIOD_MS 400

typedef struct {
    bool inited;
    bool online;
    bus_info_payload_t info;
    vfd_status_t status;
    uint32_t ok_count;
    uint32_t fail_count;
} vfd_link_ctx_t;

static vfd_link_ctx_t s_ctx;

static bool request_simple(uint8_t cmd, bus_packet_t *rsp, int timeout_ms)
{
    return bus_master_request(BUS_ADDR_VFD, cmd, NULL, 0, rsp, timeout_ms);
}

bool vfd_link_init(void)
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

bool vfd_link_ping(void)
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

bool vfd_link_read_info(void)
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

bool vfd_link_read_status(void)
{
    bus_packet_t rsp;
    bool ok = request_simple(BUS_CMD_GET_STATUS, &rsp, 50);

    if (!ok || rsp.cmd != BUS_CMD_STATUS_REPLY || rsp.len < sizeof(vfd_status_t)) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    memcpy(&s_ctx.status, rsp.data, sizeof(vfd_status_t));
    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

bool vfd_link_start(void)
{
    bus_packet_t rsp;
    bool ok = request_simple(BUS_CMD_START_ACTION, &rsp, 50);

    if (!ok || rsp.cmd != BUS_CMD_ACK || rsp.len < sizeof(bus_ack_payload_t)) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    bus_ack_payload_t ack;
    memcpy(&ack, rsp.data, sizeof(ack));

    if (ack.ack_cmd != BUS_CMD_START_ACTION) {
        s_ctx.fail_count++;
        return false;
    }

    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

bool vfd_link_stop(void)
{
    bus_packet_t rsp;
    bool ok = request_simple(BUS_CMD_STOP_ACTION, &rsp, 50);

    if (!ok || rsp.cmd != BUS_CMD_ACK || rsp.len < sizeof(bus_ack_payload_t)) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    bus_ack_payload_t ack;
    memcpy(&ack, rsp.data, sizeof(ack));

    if (ack.ack_cmd != BUS_CMD_STOP_ACTION) {
        s_ctx.fail_count++;
        return false;
    }

    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

bool vfd_link_set_speed_kmh_x100(int32_t speed_x100)
{
    vfd_set_value_t req = {
        .value_type = VFD_VAL_TARGET_SPEED_KMH,
        .value_i32  = speed_x100
    };

    bus_packet_t rsp;
    bool ok = bus_master_request(BUS_ADDR_VFD,
                                 BUS_CMD_SET_VALUE,
                                 (const uint8_t *)&req,
                                 sizeof(req),
                                 &rsp,
                                 50);

    if (!ok || rsp.cmd != BUS_CMD_ACK || rsp.len < sizeof(bus_ack_payload_t)) {
        s_ctx.online = false;
        s_ctx.fail_count++;
        return false;
    }

    bus_ack_payload_t ack;
    memcpy(&ack, rsp.data, sizeof(ack));

    if (ack.ack_cmd != BUS_CMD_SET_VALUE) {
        s_ctx.fail_count++;
        return false;
    }

    s_ctx.status.target_speed_kmh_x100 = speed_x100;
    s_ctx.online = true;
    s_ctx.ok_count++;
    return true;
}

bool vfd_link_is_online(void)
{
    return s_ctx.online;
}

bool vfd_link_get_info(bus_info_payload_t *out_info)
{
    if (!out_info) return false;
    *out_info = s_ctx.info;
    return true;
}

bool vfd_link_get_status(vfd_status_t *out_status)
{
    if (!out_status) return false;
    *out_status = s_ctx.status;
    return true;
}

uint32_t vfd_link_get_ok_count(void)
{
    return s_ctx.ok_count;
}

uint32_t vfd_link_get_fail_count(void)
{
    return s_ctx.fail_count;
}

void vfd_link_task(void *arg)
{
    (void)arg;

    bool info_read = false;
    bool was_offline = false;
    TickType_t last_offline_log = 0;
    TickType_t last_status_log = 0;
    TickType_t last_ping_tick = 0;
    bool has_prev_status = false;
    vfd_status_t prev_status;

    while (1) {
        TickType_t now = xTaskGetTickCount();
        bool need_ping = (!s_ctx.online) ||
                         ((now - last_ping_tick) >= pdMS_TO_TICKS(VFD_PING_PERIOD_MS));

        if (need_ping) {
            last_ping_tick = now;
            if (!vfd_link_ping()) {
                if ((now - last_offline_log) >= pdMS_TO_TICKS(3000)) {
                    last_offline_log = now;
                    ESP_LOGW(TAG, "vfd offline (ok=%lu fail=%lu)",
                             (unsigned long)s_ctx.ok_count,
                             (unsigned long)s_ctx.fail_count);
                }
                was_offline = true;
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
        }

        if (was_offline) {
            ESP_LOGI(TAG, "vfd online again");
            was_offline = false;
        }

        if (!info_read) {
            if (vfd_link_read_info()) {
                info_read = true;
                ESP_LOGI(TAG, "vfd info ok: name=%s fw=%u.%u build=%lu",
                         s_ctx.info.name,
                         s_ctx.info.fw_major,
                         s_ctx.info.fw_minor,
                         (unsigned long)s_ctx.info.fw_build);
            }
        }

        if (vfd_link_read_status()) {
            TickType_t now = xTaskGetTickCount();
            bool changed = (!has_prev_status) ||
                           (memcmp(&prev_status, &s_ctx.status, sizeof(vfd_status_t)) != 0);
            bool periodic = ((now - last_status_log) >= pdMS_TO_TICKS(2000));

            if (changed || periodic) {
                ESP_LOGI(TAG,
                         "state=%u faults=0x%04X run_cmd=%u run_fb=%u person=%u "
                         "spd_t=%ld spd_a=%ld hz_t=%ld hz_a=%ld I=%ld P=%ld",
                         s_ctx.status.state,
                         s_ctx.status.faults,
                         s_ctx.status.run_cmd,
                         s_ctx.status.run_fb,
                         s_ctx.status.person_detected,
                         (long)s_ctx.status.target_speed_kmh_x100,
                         (long)s_ctx.status.actual_speed_kmh_x100,
                         (long)s_ctx.status.target_hz_x100,
                         (long)s_ctx.status.actual_hz_x100,
                         (long)s_ctx.status.current_a_x100,
                         (long)s_ctx.status.power_w);
                prev_status = s_ctx.status;
                has_prev_status = true;
                last_status_log = now;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}