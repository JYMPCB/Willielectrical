#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bus_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t sof;
    uint8_t ver;
    uint8_t dest;
    uint8_t src;
    uint8_t cmd;
    uint8_t seq;
    uint8_t len;
    uint8_t data[BUS_MAX_DATA_LEN];
    uint16_t crc;
} bus_packet_t;

typedef enum {
    BUS_PARSE_OK = 0,
    BUS_PARSE_ERR_NULL,
    BUS_PARSE_ERR_LEN,
    BUS_PARSE_ERR_SOF,
    BUS_PARSE_ERR_VER,
    BUS_PARSE_ERR_DATA_LEN,
    BUS_PARSE_ERR_CRC
} bus_parse_result_t;

size_t bus_proto_build_frame(uint8_t *out_buf,
                             size_t out_buf_size,
                             uint8_t dest,
                             uint8_t src,
                             uint8_t cmd,
                             uint8_t seq,
                             const uint8_t *data,
                             uint8_t data_len);

bus_parse_result_t bus_proto_parse_frame(const uint8_t *raw,
                                         size_t raw_len,
                                         bus_packet_t *pkt);

size_t bus_proto_expected_frame_len_from_header(const uint8_t *raw, size_t raw_len);
const char *bus_proto_parse_result_str(bus_parse_result_t r);

#ifdef __cplusplus
}
#endif