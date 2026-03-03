#pragma once
#include <stdint.h>
#include "driver/i2c.h"
#include "esp_err.h"

esp_err_t i2c_mgr_init(i2c_port_t port, int sda, int scl, uint32_t hz);
esp_err_t i2c_mgr_probe(i2c_port_t port, uint8_t addr);
void      i2c_mgr_scan(i2c_port_t port);