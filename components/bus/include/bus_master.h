#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "driver/uart.h"
#include "bus_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uart_port_t uart_num;
    int tx_pin;
    int rx_pin;
    int baudrate;
    uint8_t self_addr;
    uint8_t seq_counter;
    int rx_timeout_ms;
} bus_master_config_t;

bool bus_master_init(const bus_master_config_t *cfg);

bool bus_master_send(uint8_t dest,
                     uint8_t cmd,
                     const uint8_t *data,
                     uint8_t data_len,
                     uint8_t *out_seq);

bool bus_master_recv(bus_packet_t *pkt, int timeout_ms);

bool bus_master_request(uint8_t dest,
                        uint8_t cmd,
                        const uint8_t *tx_data,
                        uint8_t tx_len,
                        bus_packet_t *rx_pkt,
                        int timeout_ms);

uint8_t bus_master_next_seq(void);

#ifdef __cplusplus
}
#endif