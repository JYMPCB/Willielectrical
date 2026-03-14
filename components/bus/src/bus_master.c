#include "bus_master.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "bus_master";

static bus_master_config_t s_cfg;
static bool s_inited = false;
static SemaphoreHandle_t s_req_mutex = NULL;

static char s_last_rx_diag[160] = "none";

static void set_last_rx_diag(const char *msg)
{
    if (!msg) {
        s_last_rx_diag[0] = '\0';
        return;
    }
    strlcpy(s_last_rx_diag, msg, sizeof(s_last_rx_diag));
}

static void log_req_fail(const char *reason,
                         uint8_t dest,
                         uint8_t cmd,
                         uint8_t seq,
                         const bus_packet_t *pkt)
{
    static int64_t last_log_us = 0;
    int64_t now_us = esp_timer_get_time();
    if ((now_us - last_log_us) < 3000000) {
        return;
    }
    last_log_us = now_us;

    if (pkt) {
        ESP_LOGW(TAG,
                 "req fail: %s dest=0x%02X cmd=0x%02X seq=%u rxdiag=%s rx(src=0x%02X dst=0x%02X cmd=0x%02X seq=%u len=%u)",
                 reason,
                 (unsigned)dest,
                 (unsigned)cmd,
                 (unsigned)seq,
                 s_last_rx_diag,
                 (unsigned)pkt->src,
                 (unsigned)pkt->dest,
                 (unsigned)pkt->cmd,
                 (unsigned)pkt->seq,
                 (unsigned)pkt->len);
    } else {
        ESP_LOGW(TAG,
                 "req fail: %s dest=0x%02X cmd=0x%02X seq=%u rxdiag=%s",
                 reason,
                 (unsigned)dest,
                 (unsigned)cmd,
                 (unsigned)seq,
                 s_last_rx_diag);
    }
}

static bool uart_read_exact(uart_port_t uart_num, uint8_t *buf, size_t len, int timeout_ms)
{
    if (!buf || len == 0 || timeout_ms <= 0) {
        return false;
    }

    int64_t deadline_us = esp_timer_get_time() + ((int64_t)timeout_ms * 1000LL);
    size_t got = 0;

    while (got < len) {
        int64_t now_us = esp_timer_get_time();
        if (now_us >= deadline_us) {
            break;
        }

        int rem_ms = (int)((deadline_us - now_us + 999LL) / 1000LL);
        if (rem_ms < 1) rem_ms = 1;

        int n = uart_read_bytes(uart_num,
                                &buf[got],
                                len - got,
                                pdMS_TO_TICKS(rem_ms));
        if (n > 0) {
            got += (size_t)n;
        }
    }

    return got == len;
}

uint8_t bus_master_next_seq(void)
{
    return ++s_cfg.seq_counter;
}

bool bus_master_init(const bus_master_config_t *cfg)
{
    if (!cfg) return false;

    if (s_inited) {
        if (s_cfg.uart_num == cfg->uart_num &&
            s_cfg.tx_pin == cfg->tx_pin &&
            s_cfg.rx_pin == cfg->rx_pin &&
            s_cfg.baudrate == cfg->baudrate &&
            s_cfg.self_addr == cfg->self_addr) {
            ESP_LOGW(TAG, "already initialized, reusing existing uart=%d", s_cfg.uart_num);
            return true;
        }

        ESP_LOGE(TAG, "already initialized with different config (uart=%d)", s_cfg.uart_num);
        return false;
    }

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

    esp_err_t err = uart_driver_install(s_cfg.uart_num, 1024, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed (%s)", esp_err_to_name(err));
        return false;
    }

    err = uart_param_config(s_cfg.uart_num, &uart_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed (%s)", esp_err_to_name(err));
        return false;
    }

    err = uart_set_pin(s_cfg.uart_num, s_cfg.tx_pin, s_cfg.rx_pin,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed (%s)", esp_err_to_name(err));
        return false;
    }

    if (s_req_mutex == NULL) {
        s_req_mutex = xSemaphoreCreateMutex();
        if (s_req_mutex == NULL) {
            ESP_LOGE(TAG, "failed to create request mutex");
            return false;
        }
    }

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

    if (cmd != BUS_CMD_PING) {
        char hex[128];
        int off = 0;
        size_t nshow = frame_len < 16 ? frame_len : 16;
        for (size_t i = 0; i < nshow && off < (int)sizeof(hex); i++) {
            off += snprintf(&hex[off], sizeof(hex) - (size_t)off,
                            "%02X%s", frame[i], (i + 1 < nshow) ? " " : "");
        }

        ESP_LOGD(TAG,
                 "tx uart=%d dest=0x%02X cmd=0x%02X seq=%u len=%u bytes=%s%s",
                 s_cfg.uart_num,
                 (unsigned)dest,
                 (unsigned)cmd,
                 (unsigned)seq,
                 (unsigned)frame_len,
                 hex,
                 (frame_len > nshow) ? " ..." : "");
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

    if (timeout_ms <= 0) {
        return false;
    }

    int64_t deadline_us = esp_timer_get_time() + ((int64_t)timeout_ms * 1000LL);

    set_last_rx_diag("waiting-sof");

    // Resync on SOF in case of line noise or stale bytes.
    uint8_t sof = 0;
    bool sof_ok = false;
    uint32_t non_sof_count = 0;
    uint8_t first_non_sof[8] = {0};
    uint8_t first_non_sof_n = 0;
    while (esp_timer_get_time() < deadline_us) {
        int64_t now_us = esp_timer_get_time();
        int rem_ms = (int)((deadline_us - now_us + 999LL) / 1000LL);
        if (rem_ms < 1) rem_ms = 1;

        int n = uart_read_bytes(s_cfg.uart_num, &sof, 1, pdMS_TO_TICKS(rem_ms));
        if (n == 1) {
            if (sof == BUS_SOF) {
                sof_ok = true;
                break;
            }

            non_sof_count++;
            if (first_non_sof_n < sizeof(first_non_sof)) {
                first_non_sof[first_non_sof_n++] = sof;
            }
        }
    }

    if (!sof_ok) {
        if (non_sof_count == 0) {
            size_t buffered = 0;
            int rx_level = gpio_get_level((gpio_num_t)s_cfg.rx_pin);
            uart_get_buffered_data_len(s_cfg.uart_num, &buffered);

            char buf[160];
            snprintf(buf,
                     sizeof(buf),
                     "timeout-no-bytes rx_pin=%d level=%d buffered=%lu",
                     s_cfg.rx_pin,
                     rx_level,
                     (unsigned long)buffered);
            set_last_rx_diag(buf);
        } else {
            char buf[160];
            int off = snprintf(buf, sizeof(buf), "timeout-no-sof bytes=%lu first=", (unsigned long)non_sof_count);
            for (uint8_t i = 0; i < first_non_sof_n && off < (int)sizeof(buf); i++) {
                off += snprintf(&buf[off], sizeof(buf) - (size_t)off, "%02X", first_non_sof[i]);
                if (i + 1 < first_non_sof_n && off < (int)sizeof(buf)) {
                    off += snprintf(&buf[off], sizeof(buf) - (size_t)off, " ");
                }
            }
            set_last_rx_diag(buf);
        }
        return false;
    }

    uint8_t hdr[7];
    hdr[0] = BUS_SOF;

    int rem_total_ms = (int)((deadline_us - esp_timer_get_time() + 999LL) / 1000LL);
    if (rem_total_ms < 1 || !uart_read_exact(s_cfg.uart_num, &hdr[1], 6, rem_total_ms)) {
        set_last_rx_diag("timeout-header");
        return false;
    }

    size_t exp_len = bus_proto_expected_frame_len_from_header(hdr, sizeof(hdr));
    if (exp_len < BUS_MIN_FRAME_LEN || exp_len > BUS_MAX_FRAME_LEN) {
        char buf[96];
        snprintf(buf, sizeof(buf), "bad-len-from-header rawlen=%u", (unsigned)hdr[6]);
        set_last_rx_diag(buf);
        return false;
    }

    uint8_t raw[BUS_MAX_FRAME_LEN];
    memcpy(raw, hdr, sizeof(hdr));

    size_t remain = exp_len - sizeof(hdr);
    rem_total_ms = (int)((deadline_us - esp_timer_get_time() + 999LL) / 1000LL);
    if (rem_total_ms < 1 || !uart_read_exact(s_cfg.uart_num, &raw[7], remain, rem_total_ms)) {
        set_last_rx_diag("timeout-body");
        return false;
    }

    bus_parse_result_t parse = bus_proto_parse_frame(raw, exp_len, pkt);
    if (parse != BUS_PARSE_OK) {
        char buf[96];
        snprintf(buf, sizeof(buf), "parse-%s", bus_proto_parse_result_str(parse));
        set_last_rx_diag(buf);
        return false;
    }

    set_last_rx_diag("ok");
    return true;
}

bool bus_master_request(uint8_t dest,
                        uint8_t cmd,
                        const uint8_t *tx_data,
                        uint8_t tx_len,
                        bus_packet_t *rx_pkt,
                        int timeout_ms)
{
    bool ok = false;

    if (!s_inited || !rx_pkt) {
        log_req_fail("not-inited-or-null-rx", dest, cmd, 0, NULL);
        return false;
    }

    if (s_req_mutex == NULL || xSemaphoreTake(s_req_mutex, pdMS_TO_TICKS(timeout_ms + 50)) != pdTRUE) {
        log_req_fail("lock-timeout", dest, cmd, 0, NULL);
        return false;
    }

    // Discard stale bytes from a previous transaction before issuing a new request.
    uart_flush_input(s_cfg.uart_num);

    uint8_t seq = 0;
    if (!bus_master_send(dest, cmd, tx_data, tx_len, &seq)) {
        log_req_fail("tx-failed", dest, cmd, seq, NULL);
        goto done;
    }

    bus_packet_t pkt;
    if (!bus_master_recv(&pkt, timeout_ms)) {
        log_req_fail("rx-timeout-or-parse", dest, cmd, seq, NULL);
        goto done;
    }

    if (pkt.dest != s_cfg.self_addr) {
        log_req_fail("rx-dest-mismatch", dest, cmd, seq, &pkt);
        goto done;
    }

    if (pkt.src != dest) {
        log_req_fail("rx-src-mismatch", dest, cmd, seq, &pkt);
        goto done;
    }

    if (pkt.seq != seq) {
        log_req_fail("rx-seq-mismatch", dest, cmd, seq, &pkt);
        goto done;
    }

    *rx_pkt = pkt;
    ok = true;

done:
    xSemaphoreGive(s_req_mutex);
    return ok;
}