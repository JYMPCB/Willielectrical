#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t bus_crc16_modbus(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif