#include "i2c_mgr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


esp_err_t i2c_mgr_init(i2c_port_t port, int sda, int scl, uint32_t hz)
{
  // Si el driver ya está instalado, NO lo podés reconfigurar.
  // Por eso este init debe ejecutarse primero.
  i2c_config_t conf = {};
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = (gpio_num_t)sda;
  conf.scl_io_num = (gpio_num_t)scl;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = hz;

  esp_err_t err = i2c_param_config(port, &conf);
  if(err != ESP_OK) return err;

  err = i2c_driver_install(port, conf.mode, 0, 0, 0);
  if(err == ESP_ERR_INVALID_STATE) return ESP_OK; // ya instalado
  return err;
}

esp_err_t i2c_mgr_probe(i2c_port_t port, uint8_t addr)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_stop(cmd);
  esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(30));
  i2c_cmd_link_delete(cmd);
  return err;
}

void i2c_mgr_scan(i2c_port_t port)
{
  for(int a=1; a<127; a++){
    if(i2c_mgr_probe(port, a) == ESP_OK){
      Serial.printf("[I2C] found device at 0x%02X\n", a);
    }
  }
}