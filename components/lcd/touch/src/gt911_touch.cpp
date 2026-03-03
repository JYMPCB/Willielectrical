#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lcd_touch_gt911.h"
#include "gt911_touch.h"

#define CONFIG_LCD_HRES 1024
#define CONFIG_LCD_VRES 600

static const char *TAG = "example";
static constexpr uint32_t GT911_I2C_SCL_HZ = 400000;

static esp_lcd_touch_handle_t tp;
static esp_lcd_panel_io_handle_t tp_io_handle;
static i2c_master_bus_handle_t s_i2c_bus = nullptr;

gt911_touch::gt911_touch(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin, int8_t int_pin)
{
    _sda = sda_pin;
    _scl = scl_pin;
    _rst = rst_pin;
    _int = int_pin;
}

void gt911_touch::begin()
{
    esp_err_t err;

    if (s_i2c_bus == nullptr) {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = (gpio_num_t)_sda,
            .scl_io_num = (gpio_num_t)_scl,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = true,
                .allow_pd = false,
            },
        };

        err = i2c_new_master_bus(&bus_config, &s_i2c_bus);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
            return;
        }
    }

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    tp_io_config.scl_speed_hz = GT911_I2C_SCL_HZ;
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    err = esp_lcd_new_panel_io_i2c(s_i2c_bus, &tp_io_config, &tp_io_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_i2c failed: %s", esp_err_to_name(err));
        return;
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_LCD_HRES,
        .y_max = CONFIG_LCD_VRES,
        .rst_gpio_num = (gpio_num_t)_rst,
        .int_gpio_num = (gpio_num_t)_int,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller gt911");
    err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_touch_new_i2c_gt911 failed: %s", esp_err_to_name(err));
        return;
    }
}

bool gt911_touch::getTouch(uint16_t *x, uint16_t *y)
{
    /*esp_lcd_touch_read_data(tp);
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, x, y, touch_strength, &touch_cnt, 1);

    return touchpad_pressed;*/
    if (!tp) return false;

    uint16_t tx, ty;
    uint8_t cnt = 0;

    esp_lcd_touch_read_data(tp);
    bool pressed = esp_lcd_touch_get_coordinates(
        tp, &tx, &ty, nullptr, &cnt, 1
    );

    if (!pressed || cnt == 0) return false;

    // Clamp seguro
    if (tx >= CONFIG_LCD_HRES) tx = CONFIG_LCD_HRES - 1;
    if (ty >= CONFIG_LCD_VRES) ty = CONFIG_LCD_VRES - 1;

    *x = tx;
    *y = ty;
    return true;    
}

void gt911_touch::set_rotation(uint8_t r){
switch(r){
    case 0:
        esp_lcd_touch_set_swap_xy(tp, false);   
        esp_lcd_touch_set_mirror_x(tp, false);
        esp_lcd_touch_set_mirror_y(tp, false);
        break;
    case 1:
        esp_lcd_touch_set_swap_xy(tp, false);
        esp_lcd_touch_set_mirror_x(tp, true);
        esp_lcd_touch_set_mirror_y(tp, true);
        break;
    case 2:
        esp_lcd_touch_set_swap_xy(tp, false);   
        esp_lcd_touch_set_mirror_x(tp, false);
        esp_lcd_touch_set_mirror_y(tp, false);
        break;
    case 3:
        esp_lcd_touch_set_swap_xy(tp, false);   
        esp_lcd_touch_set_mirror_x(tp, true);
        esp_lcd_touch_set_mirror_y(tp, true);
        break;
    }

}