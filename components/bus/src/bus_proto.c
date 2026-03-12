#include "bus_proto.h"
#include "bus_crc.h"
#include <string.h>

size_t bus_proto_expected_frame_len_from_header(const uint8_t *raw, size_t raw_len)
{
    if (!raw || raw_len < 7) {
        return 0;
    }

    uint8_t data_len = raw[6];
    return (size_t)(7 + data_len + 2);
}

size_t bus_proto_build_frame(uint8_t *out_buf,
                             size_t out_buf_size,
                             uint8_t dest,
                             uint8_t src,
                             uint8_t cmd,
                             uint8_t seq,
                             const uint8_t *data,
                             uint8_t data_len)
{
    if (!out_buf) return 0;
    if (data_len > BUS_MAX_DATA_LEN) return 0;

    size_t total_len = (size_t)(7 + data_len + 2);
    if (out_buf_size < total_len) return 0;

    out_buf[0] = BUS_SOF;
    out_buf[1] = BUS_PROTO_VER;
    out_buf[2] = dest;
    out_buf[3] = src;
    out_buf[4] = cmd;
    out_buf[5] = seq;
    out_buf[6] = data_len;

    if (data_len > 0) {
        if (!data) return 0;
        memcpy(&out_buf[7], data, data_len);
    }

    uint16_t crc = bus_crc16_modbus(&out_buf[1], (size_t)(6 + data_len));
    out_buf[7 + data_len] = (uint8_t)(crc & 0xFF);
    out_buf[8 + data_len] = (uint8_t)((crc >> 8) & 0xFF);

    return total_len;
}

bus_parse_result_t bus_proto_parse_frame(const uint8_t *raw,
                                         size_t raw_len,
                                         bus_packet_t *pkt)
{
    if (!raw || !pkt) {
        return BUS_PARSE_ERR_NULL;
    }

    if (raw_len < BUS_MIN_FRAME_LEN) {
        return BUS_PARSE_ERR_LEN;
    }

    if (raw[0] != BUS_SOF) {
        return BUS_PARSE_ERR_SOF;
    }

    if (raw[1] != BUS_PROTO_VER) {
        return BUS_PARSE_ERR_VER;
    }

    uint8_t data_len = raw[6];
    if (data_len > BUS_MAX_DATA_LEN) {
        return BUS_PARSE_ERR_DATA_LEN;
    }

    size_t expected_len = (size_t)(7 + data_len + 2);
    if (raw_len != expected_len) {
        return BUS_PARSE_ERR_LEN;
    }

    uint16_t rx_crc = (uint16_t)raw[7 + data_len] |
                      ((uint16_t)raw[8 + data_len] << 8);

    uint16_t calc_crc = bus_crc16_modbus(&raw[1], (size_t)(6 + data_len));

    if (rx_crc != calc_crc) {
        return BUS_PARSE_ERR_CRC;
    }

    pkt->sof  = raw[0];
    pkt->ver  = raw[1];
    pkt->dest = raw[2];
    pkt->src  = raw[3];
    pkt->cmd  = raw[4];
    pkt->seq  = raw[5];
    pkt->len  = raw[6];
    pkt->crc  = rx_crc;

    if (data_len > 0) {
        memcpy(pkt->data, &raw[7], data_len);
    }

    return BUS_PARSE_OK;
}

const char *bus_proto_parse_result_str(bus_parse_result_t r)
{
    switch (r) {
        case BUS_PARSE_OK:           return "BUS_PARSE_OK";
        case BUS_PARSE_ERR_NULL:     return "BUS_PARSE_ERR_NULL";
        case BUS_PARSE_ERR_LEN:      return "BUS_PARSE_ERR_LEN";
        case BUS_PARSE_ERR_SOF:      return "BUS_PARSE_ERR_SOF";
        case BUS_PARSE_ERR_VER:      return "BUS_PARSE_ERR_VER";
        case BUS_PARSE_ERR_DATA_LEN: return "BUS_PARSE_ERR_DATA_LEN";
        case BUS_PARSE_ERR_CRC:      return "BUS_PARSE_ERR_CRC";
        default:                     return "BUS_PARSE_ERR_UNKNOWN";
    }
}