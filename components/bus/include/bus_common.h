#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUS_PROTO_VER               0x01
#define BUS_SOF                     0xAA
#define BUS_MAX_DATA_LEN            64
#define BUS_MIN_FRAME_LEN           9
#define BUS_MAX_FRAME_LEN           (BUS_MIN_FRAME_LEN + BUS_MAX_DATA_LEN)

typedef enum {
    BUS_ADDR_HANDLEBAR   = 0x01,
    BUS_ADDR_VFD         = 0x02,
    BUS_ADDR_INCLINE     = 0x03,
    BUS_ADDR_BLE_AUDIO   = 0x04,
    BUS_ADDR_SENSOR_PACK = 0x05,
    BUS_ADDR_AUX_IO      = 0x06,

    BUS_ADDR_GROUP       = 0x0F,

    BUS_ADDR_HMI_MASTER  = 0xF0,
    BUS_ADDR_BROADCAST   = 0xFE,
    BUS_ADDR_INVALID     = 0xFF
} bus_addr_t;

typedef enum {
    BUS_CMD_PING          = 0x01,
    BUS_CMD_PONG          = 0x02,
    BUS_CMD_GET_INFO      = 0x03,
    BUS_CMD_INFO_REPLY    = 0x04,
    BUS_CMD_GET_STATUS    = 0x05,
    BUS_CMD_STATUS_REPLY  = 0x06,
    BUS_CMD_ACK           = 0x07,
    BUS_CMD_NACK          = 0x08,
    BUS_CMD_RESET_MODULE  = 0x09,
    BUS_CMD_BOOTLOADER    = 0x0A,

    BUS_CMD_SET_MODE      = 0x10,
    BUS_CMD_SET_VALUE     = 0x11,
    BUS_CMD_START_ACTION  = 0x12,
    BUS_CMD_STOP_ACTION   = 0x13,
    BUS_CMD_CLEAR_EVENTS  = 0x14,

    BUS_CMD_GET_EVENTS    = 0x20,
    BUS_CMD_EVENTS_REPLY  = 0x21,

    BUS_CMD_GET_DIAG      = 0x30,
    BUS_CMD_DIAG_REPLY    = 0x31,

    BUS_CMD_GET_CONFIG    = 0x40,
    BUS_CMD_CONFIG_REPLY  = 0x41,
    BUS_CMD_SET_CONFIG    = 0x42,
    BUS_CMD_SAVE_CONFIG   = 0x43,
} bus_cmd_t;

typedef enum {
    BUS_MOD_UNKNOWN    = 0x00,
    BUS_MOD_HANDLEBAR  = 0x01,
    BUS_MOD_VFD        = 0x02,
    BUS_MOD_INCLINE    = 0x03,
    BUS_MOD_BLE_AUDIO  = 0x04,
    BUS_MOD_SENSOR     = 0x05,
    BUS_MOD_AUX_IO     = 0x06
} bus_module_type_t;

typedef enum {
    BUS_STATE_INIT       = 0,
    BUS_STATE_IDLE       = 1,
    BUS_STATE_READY      = 2,
    BUS_STATE_RUNNING    = 3,
    BUS_STATE_BUSY       = 4,
    BUS_STATE_WARN       = 5,
    BUS_STATE_ERROR      = 6,
    BUS_STATE_BOOTLOADER = 7
} bus_module_state_t;

typedef enum {
    BUS_ERR_NONE         = 0x00,
    BUS_ERR_BAD_CRC      = 0x01,
    BUS_ERR_BAD_LEN      = 0x02,
    BUS_ERR_BAD_CMD      = 0x03,
    BUS_ERR_BAD_PARAM    = 0x04,
    BUS_ERR_BUSY         = 0x05,
    BUS_ERR_UNSUPPORTED  = 0x06,
    BUS_ERR_INTERNAL     = 0x07,
    BUS_ERR_TIMEOUT      = 0x08
} bus_error_t;

typedef struct __attribute__((packed)) {
    uint8_t ack_cmd;
} bus_ack_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t nack_cmd;
    uint8_t error_code;
} bus_nack_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t  module_type;
    uint8_t  proto_ver;
    uint8_t  fw_major;
    uint8_t  fw_minor;
    uint32_t fw_build;
    char     name[16];
} bus_info_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t alive;
    uint8_t state;
    uint8_t faults;
    uint8_t events_pending;
} bus_status_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint8_t  param;
    uint16_t time_ms;
} bus_event_item_t;

#ifdef __cplusplus
}
#endif