#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VFD_STATE_STOPPED = 0,
    VFD_STATE_READY   = 1,
    VFD_STATE_RUNNING = 2,
    VFD_STATE_FAULT   = 3
} vfd_state_t;

typedef enum {
    VFD_VAL_NONE             = 0,
    VFD_VAL_TARGET_SPEED_KMH = 1
} vfd_value_type_t;

typedef enum {
    VFD_FAULT_NONE         = 0,
    VFD_FAULT_DRIVE        = (1u << 0),
    VFD_FAULT_SENSOR       = (1u << 1),
    VFD_FAULT_OVERCURRENT  = (1u << 2),
    VFD_FAULT_COMMS        = (1u << 3),
    VFD_FAULT_SAFETY       = (1u << 4)
} vfd_fault_flags_t;

typedef struct __attribute__((packed)) {
    uint8_t value_type;
    int32_t value_i32;
} vfd_set_value_t;

typedef struct __attribute__((packed)) {
    uint8_t  alive;
    uint8_t  state;
    uint16_t faults;

    uint8_t  run_cmd;
    uint8_t  run_fb;
    uint8_t  person_detected;
    uint8_t  reserved0;

    int32_t  target_speed_kmh_x100;
    int32_t  actual_speed_kmh_x100;

    int32_t  target_hz_x100;
    int32_t  actual_hz_x100;

    int32_t  current_a_x100;
    int32_t  power_w;
} vfd_status_t;

#ifdef __cplusplus
}
#endif