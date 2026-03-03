#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

typedef struct {
  i2c_master_bus_handle_t i2c_bus;
  uint8_t i2c_addr;       // 0x18
  uint32_t i2c_scl_hz;    // 100k/400k
  bool master_mode;       // false (codec slave)
  bool use_mclk;          // true
  bool invert_mclk;       // false
  bool digital_mic;       // false
  int mclk_div;           // 384 (como el ejemplo) o 256
} es8311_cfg_t;

esp_err_t es8311_init_arduino(const es8311_cfg_t *cfg, int sample_rate_hz);
esp_err_t es8311_set_volume_reg(uint8_t vol_reg_0_255);  // escribe DAC_REG32
esp_err_t es8311_set_mute(bool mute);