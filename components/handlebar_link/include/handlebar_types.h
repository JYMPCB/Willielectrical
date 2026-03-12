#pragma once

#include <stdint.h>
#include "bus_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HANDLE_MODE_SPEED = 0,
    HANDLE_MODE_PACE  = 1
} handlebar_mode_t;

typedef enum {
    HANDLE_EV_NONE  = 0,
    HANDLE_EV_START = 1,
    HANDLE_EV_STOP  = 2,
    HANDLE_EV_MODE  = 3,
    HANDLE_EV_UP    = 4,
    HANDLE_EV_DOWN  = 5
} handlebar_event_type_t;

typedef enum {
    HANDLE_BTNF_START = (1u << 0),
    HANDLE_BTNF_STOP  = (1u << 1),
    HANDLE_BTNF_MODE  = (1u << 2),
    HANDLE_BTNF_UP    = (1u << 3),
    HANDLE_BTNF_DOWN  = (1u << 4),
} handlebar_btn_flags_t;

typedef struct __attribute__((packed)) {
    uint8_t alive;
    uint8_t state;
    uint8_t faults;
    uint8_t events_pending;
    uint8_t mode;
    uint8_t btn_flags;
    uint8_t led_enabled;
    uint8_t brightness;
} handlebar_status_t;

typedef struct __attribute__((packed)) {
    uint8_t mode;
} handlebar_set_mode_t;

typedef struct __attribute__((packed)) {
    uint8_t count;
    bus_event_item_t items[15];
} handlebar_events_reply_t;

#ifdef __cplusplus
}
#endif