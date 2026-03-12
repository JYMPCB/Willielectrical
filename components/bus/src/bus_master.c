#include "bus_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "bus_master";

static bus_master_config_t s_cfg;
static bool s_inited = false;

uint8_t bus_master_next_seq(void)
{
    return ++s_cfg.seq_counter;
}

bool bus_master_init(const bus_master_config_t *cfg)
{
    if (!cfg) return false;

    memset(&s_cfg, 0, sizeof(s_cfg));
    s_cfg = *cfg;

    uart_config_t uart_cfg = {
        .baud_rate = s_cfg.baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(s_cfg.uart_num, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(s_cfg.uart_num, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(s_cfg.uart_num, s_cfg.tx_pin, s_cfg.rx_pin,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    s_inited = true;
    ESP_LOGI(TAG, "init ok uart=%d tx=%d rx=%d baud=%d addr=0x%02X",
             s_cfg.uart_num, s_cfg.tx_pin, s_cfg.rx_pin,
             s_cfg.baudrate, s_cfg.self_addr);

    return true;
}

bool bus_master_send(uint8_t dest,
                     uint8_t cmd,
                     const uint8_t *data,
                     uint8_t data_len,
                     uint8_t *out_seq)
{
    if (!s_inited) return false;

    uint8_t seq = bus_master_next_seq();
    uint8_t frame[BUS_MAX_FRAME_LEN];

    size_t frame_len = bus_proto_build_frame(frame, sizeof(frame),
                                            dest, s_cfg.self_addr,
                                            cmd, seq,
                                            data, data_len);
    if (frame_len == 0) {
        return false;
    }

    int written = uart_write_bytes(s_cfg.uart_num, frame, frame_len);
    if (written != (int)frame_len) {
        return false;
    }

    uart_wait_tx_done(s_cfg.uart_num, pdMS_TO_TICKS(20));

    if (out_seq) {
        *out_seq = seq;
    }

    return true;
}

bool bus_master_recv(bus_packet_t *pkt, int timeout_ms)
{
    if (!s_inited || !pkt) return false;

    uint8_t hdr[7];
    int n = uart_read_bytes(s_cfg.uart_num, hdr, sizeof(hdr), pdMS_TO_TICKS(timeout_ms));
    if (n != (int)sizeof(hdr)) {
        return false;
    }

    size_t exp_len = bus_proto_expected_frame_len_from_header(hdr, sizeof(hdr));
    if (exp_len < BUS_MIN_FRAME_LEN || exp_len > BUS_MAX_FRAME_LEN) {
        return false;
    }

    uint8_t raw[BUS_MAX_FRAME_LEN];
    memcpy(raw, hdr, sizeof(hdr));

    size_t remain = exp_len - sizeof(hdr);
    n = uart_read_bytes(s_cfg.uart_num, &raw[7], remain, pdMS_TO_TICKS(timeout_ms));
    if (n != (int)remain) {
        return false;
    }

    return bus_proto_parse_frame(raw, exp_len, pkt) == BUS_PARSE_OK;
}

bool bus_master_request(uint8_t dest,
                        uint8_t cmd,
                        const uint8_t *tx_data,
                        uint8_t tx_len,
                        bus_packet_t *rx_pkt,
                        int timeout_ms)
{
    if (!s_inited || !rx_pkt) return false;

    uint8_t seq = 0;
    if (!bus_master_send(dest, cmd, tx_data, tx_len, &seq)) {
        return false;
    }

    bus_packet_t pkt;
    if (!bus_master_recv(&pkt, timeout_ms)) {
        return false;
    }

    if (pkt.dest != s_cfg.self_addr) {
        return false;
    }

    if (pkt.src != dest) {
        return false;
    }

    if (pkt.seq != seq) {
        return false;
    }

    *rx_pkt = pkt;
    return true;
}