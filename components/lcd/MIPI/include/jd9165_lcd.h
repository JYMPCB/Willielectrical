#ifndef _JD9165_LCD_H
#define _JD9165_LCD_H

#include <stdio.h>
#include <stdint.h>

// 👇 necesitás los tipos de esp_lcd / jd9165
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_vendor.h"   // según tu SDK
#include "esp_lcd_jd9165.h"                 
#include "esp_err.h"

class jd9165_lcd
{
public:
    jd9165_lcd(int8_t lcd_rst);

    void begin();
    void example_bsp_enable_dsi_phy_power();
    void example_bsp_init_lcd_backlight();
    void example_bsp_set_lcd_backlight(uint32_t level);
    esp_err_t lcd_draw_bitmap(uint16_t x_start, uint16_t y_start,
                              uint16_t x_end, uint16_t y_end, uint16_t *color_data);
    void draw16bitbergbbitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *color_data);
    void fillScreen(uint16_t color);
    void te_on();
    void te_off();
    uint16_t width();
    uint16_t height();
    esp_lcd_panel_handle_t get_panel_handle() const { return panel_handle; }

private:
    int8_t _lcd_rst;

    // ✅ Handles persistentes (antes eran locales)
    esp_lcd_dsi_bus_handle_t   mipi_dsi_bus  = nullptr;
    esp_lcd_panel_io_handle_t  io_handle     = nullptr;
    esp_lcd_panel_handle_t     panel_handle  = nullptr;

    // ✅ Configs persistentes (evita punteros a stack)
    esp_lcd_dpi_panel_config_t dpi_config{};
    jd9165_vendor_config_t     vendor_config{};
    esp_lcd_panel_dev_config_t panel_config{};
};

#endif
