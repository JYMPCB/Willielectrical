#include "es8311.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "es8311_reg.h"  

static const char *TAG = "es8311";

static es8311_cfg_t s_cfg;
static i2c_master_dev_handle_t s_dev = nullptr;

// --- Tabla coeff_div y config_sample: copiada del driver oficial (recortada a lo esencial) ---
struct coeff_div_t {
  uint32_t mclk;
  uint32_t rate;
  uint8_t pre_div;
  uint8_t pre_multi;
  uint8_t adc_div;
  uint8_t dac_div;
  uint8_t fs_mode;
  uint8_t lrck_h;
  uint8_t lrck_l;
  uint8_t bclk_div;
  uint8_t adc_osr;
  uint8_t dac_osr;
};

// IMPORTANTE: para no pegarte 500 líneas acá, dejé solo entradas comunes.
// Si querés 100% cobertura, copiamos toda la tabla completa del es8311.c (la que me pasaste).
static const coeff_div_t coeff_div[] = {
  // mclk, rate, pre_div, mult, adc_div, dac_div, fs, lrH, lrL, bclk, adc_osr, dac_osr
  { 6144000,  16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20 }, // 16k @ 6.144MHz (384x)
  { 12288000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10 }, // 48k @ 12.288MHz (256x)
  { 6144000,  48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10 }, // 48k @ 6.144MHz (128x) (por si probás)
};

static int find_coeff(uint32_t mclk, uint32_t rate) {
  for (int i = 0; i < (int)(sizeof(coeff_div)/sizeof(coeff_div[0])); i++) {
    if (coeff_div[i].mclk == mclk && coeff_div[i].rate == rate) return i;
  }
  return -1;
}

// --- I2C helpers (bus ya instalado por GT911) ---
static esp_err_t i2c_write_reg(uint8_t reg, uint8_t val)
{
  if (!s_dev) return ESP_ERR_INVALID_STATE;
  uint8_t buf[2] = { reg, val };
  return i2c_master_transmit(s_dev, buf, sizeof(buf), 50);
}

static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *out)
{
  if (!s_dev) return ESP_ERR_INVALID_STATE;
  return i2c_master_transmit_receive(s_dev, &reg, 1, out, 1, 50);
}

// --- Lo importante: config_sample() del driver oficial ---
static esp_err_t es8311_config_sample(int sample_rate)
{
  int mclk = sample_rate * s_cfg.mclk_div; // igual que driver oficial
  int idx = find_coeff((uint32_t)mclk, (uint32_t)sample_rate);
  if (idx < 0) {
    ESP_LOGE(TAG, "No coeff for rate=%d mclk=%d (mclk_div=%d)", sample_rate, mclk, s_cfg.mclk_div);
    return ESP_ERR_NOT_SUPPORTED;
  }

  uint8_t regv = 0;

  // REG02: pre_div + pre_multi
  // driver: regv = (reg02 & 0x07) | ((pre_div-1)<<5) | (pre_multi_code<<3)
  uint8_t reg02 = 0;
  (void)i2c_read_reg(ES8311_CLK_MANAGER_REG02, &reg02);
  regv = (reg02 & 0x07);
  regv |= (uint8_t)((coeff_div[idx].pre_div - 1) << 5);

  uint8_t mult_code = 0;
  switch (coeff_div[idx].pre_multi) {
    case 1: mult_code = 0; break;
    case 2: mult_code = 1; break;
    case 4: mult_code = 2; break;
    case 8: mult_code = 3; break;
    default: mult_code = 0; break;
  }
  if (!s_cfg.use_mclk) mult_code = 3;
  regv |= (uint8_t)(mult_code << 3);
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG02, regv));

  // REG05: adc_div/dac_div
  regv = 0;
  regv |= (uint8_t)((coeff_div[idx].adc_div - 1) << 4);
  regv |= (uint8_t)((coeff_div[idx].dac_div - 1) << 0);
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG05, regv));

  // REG03: fs_mode + adc_osr (preserva bit7)
  uint8_t reg03 = 0;
  (void)i2c_read_reg(ES8311_CLK_MANAGER_REG03, &reg03);
  regv = (reg03 & 0x80);
  regv |= (uint8_t)(coeff_div[idx].fs_mode << 6);
  regv |= (uint8_t)(coeff_div[idx].adc_osr);
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG03, regv));

  // REG04: dac_osr (preserva bit7)
  uint8_t reg04 = 0;
  (void)i2c_read_reg(ES8311_CLK_MANAGER_REG04, &reg04);
  regv = (reg04 & 0x80);
  regv |= (uint8_t)(coeff_div[idx].dac_osr);
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG04, regv));

  // REG07: lrck_h (preserva bits7..6)
  uint8_t reg07 = 0;
  (void)i2c_read_reg(ES8311_CLK_MANAGER_REG07, &reg07);
  regv = (reg07 & 0xC0);
  regv |= (uint8_t)(coeff_div[idx].lrck_h);
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG07, regv));

  // REG08: lrck_l
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG08, (uint8_t)coeff_div[idx].lrck_l));

  // REG06: bclk_div (preserva bits7..5)
  uint8_t reg06 = 0;
  (void)i2c_read_reg(ES8311_CLK_MANAGER_REG06, &reg06);
  regv = (reg06 & 0xE0);
  if (coeff_div[idx].bclk_div < 19) regv |= (uint8_t)(coeff_div[idx].bclk_div - 1);
  else regv |= (uint8_t)(coeff_div[idx].bclk_div);
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG06, regv));

  return ESP_OK;
}

// --- “open + start” (recorte del driver oficial, suficiente para DAC) ---
static esp_err_t es8311_open_and_start()
{
  // open(): base init igual al driver
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG01, 0x30));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG02, 0x00));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG03, 0x10));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_ADC_REG16,         0x24));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG04, 0x10));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG05, 0x00));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG0B,      0x00));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG0C,      0x00));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG10,      0x1F));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG11,      0x7F));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_RESET_REG00,       0x80));

  // start(): reset reg00 master/slave + clock source
  uint8_t reg00 = 0x80;
  if (s_cfg.master_mode) reg00 |= 0x40;
  else reg00 &= 0xBF;
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_RESET_REG00, reg00));

  uint8_t reg01 = 0x3F;
  if (s_cfg.use_mclk) reg01 &= 0x7F;
  else reg01 |= 0x80;
  if (s_cfg.invert_mclk) reg01 |= 0x40;
  else reg01 &= (uint8_t)~0x40;
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_CLK_MANAGER_REG01, reg01));

  // Habilitar DAC path / system regs (igual driver)
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_ADC_REG17,   0xBF));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG0E,0x02));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG12,0x00));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG14,0x1A));

  // dmic on/off
  uint8_t reg14 = 0;
  (void)i2c_read_reg(ES8311_SYSTEM_REG14, &reg14);
  if (s_cfg.digital_mic) reg14 |= 0x40;
  else reg14 &= (uint8_t)~0x40;
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG14, reg14));

  ESP_ERROR_CHECK(i2c_write_reg(ES8311_SYSTEM_REG0D, 0x01));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_ADC_REG15,    0x40));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_DAC_REG37,    0x08));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_GP_REG45,     0x00));

  // Unmute + volumen razonable
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_DAC_REG31, 0x00));
  ESP_ERROR_CHECK(i2c_write_reg(ES8311_DAC_REG32, 0x00));

  return ESP_OK;
}

esp_err_t es8311_init_arduino(const es8311_cfg_t *cfg, int sample_rate_hz)
{
  if (!cfg || !cfg->i2c_bus) return ESP_ERR_INVALID_ARG;
  s_cfg = *cfg;

  if (s_dev) {
    (void)i2c_master_bus_rm_device(s_dev);
    s_dev = nullptr;
  }

  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = s_cfg.i2c_addr,
    .scl_speed_hz = (s_cfg.i2c_scl_hz > 0) ? s_cfg.i2c_scl_hz : 100000,
    .scl_wait_us = 0,
    .flags = {
      .disable_ack_check = 0,
    },
  };

  esp_err_t err = i2c_master_bus_add_device(s_cfg.i2c_bus, &dev_cfg, &s_dev);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
    return err;
  }

  // reset soft como en tu dump: reg00=0x80 está bien, no lo tocamos más que open/start
  ESP_ERROR_CHECK(es8311_open_and_start());

  // CLAVE: configurar sample-rate/clock (lo que a vos te faltaba)
  ESP_ERROR_CHECK(es8311_config_sample(sample_rate_hz));

  return ESP_OK;
}

esp_err_t es8311_set_volume_reg(uint8_t vol_reg_0_255)
{
  return i2c_write_reg(ES8311_DAC_REG32, vol_reg_0_255);
}

esp_err_t es8311_set_mute(bool mute)
{
  uint8_t reg = 0;
  esp_err_t e = i2c_read_reg(ES8311_DAC_REG31, &reg);
  if (e != ESP_OK) return e;
  reg &= 0x9F;
  if (mute) reg |= 0x60;
  return i2c_write_reg(ES8311_DAC_REG31, reg);
}