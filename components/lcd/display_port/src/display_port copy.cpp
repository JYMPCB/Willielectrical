#include "display_port.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "lv_conf.h"
#include "lvgl.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"

#include "pins_config.h"
#include "jd9165_lcd.h"
#include "gt911_touch.h"
#include "app_ui.h"

static const char *TAG = "display_port";

/* Objetos de hardware */
static jd9165_lcd lcd(LCD_RST);
static gt911_touch touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);

/* LVGL 9 */
static lv_display_t *s_display = nullptr;
static lv_indev_t   *s_indev   = nullptr;

/* Estado */
static bool s_flush_enabled = true;

/* Resolución */
#ifndef LCD_H_RES
#define LCD_H_RES 1024
#endif

#ifndef LCD_V_RES
#define LCD_V_RES 600
#endif

/* ===============================================================
 * Buffers completos, igual concepto que en LVGL 8
 * =============================================================== */
#define DRAW_BUF_PIXELS  (LCD_H_RES * LCD_V_RES)

static lv_color_t *s_buf1 = nullptr;
static lv_color_t *s_buf2 = nullptr;

/*---------------------------------------------------------------
 * Flush callback LVGL 9
 *--------------------------------------------------------------*/
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    LV_UNUSED(disp);

    if(!s_flush_enabled) {
        lv_display_flush_ready(disp);
        return;
    }

    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;

    /* Recorte defensivo */
    if(x2 < 0 || y2 < 0 || x1 >= LCD_H_RES || y1 >= LCD_V_RES) {
        lv_display_flush_ready(disp);
        return;
    }

    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 >= LCD_H_RES) x2 = LCD_H_RES - 1;
    if(y2 >= LCD_V_RES) y2 = LCD_V_RES - 1;

    esp_err_t err = lcd.lcd_draw_bitmap(
        (uint16_t)x1,
        (uint16_t)y1,
        (uint16_t)(x2 + 1),
        (uint16_t)(y2 + 1),
        (uint16_t *)px_map
    );

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "lcd_draw_bitmap failed: %s", esp_err_to_name(err));
        lv_display_flush_ready(disp);
    }
}

/*---------------------------------------------------------------
 * Callback async del panel: avisa a LVGL que terminó el flush
 *--------------------------------------------------------------*/
static bool lcd_color_trans_done_cb(esp_lcd_panel_handle_t panel,
                                    esp_lcd_dpi_panel_event_data_t *edata,
                                    void *user_ctx)
{
    (void)panel;
    (void)edata;

    lv_display_t *disp = (lv_display_t *)user_ctx;
    if(disp) {
        lv_display_flush_ready(disp);
    }
    return false;
}

/*---------------------------------------------------------------
 * Touch read callback LVGL 9
 *--------------------------------------------------------------*/
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    LV_UNUSED(indev);

    uint16_t x = 0;
    uint16_t y = 0;

    if(touch.getTouch(&x, &y)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = (lv_coord_t)x;
        data->point.y = (lv_coord_t)y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/*---------------------------------------------------------------
 * Rotación
 *--------------------------------------------------------------*/
static void display_set_rotation(lv_display_rotation_t rot)
{
    if(!s_display) return;

    lv_display_set_rotation(s_display, rot);

    switch(rot) {
        case LV_DISPLAY_ROTATION_0:
            touch.set_rotation(0);
            break;
        case LV_DISPLAY_ROTATION_90:
            touch.set_rotation(1);
            break;
        case LV_DISPLAY_ROTATION_180:
            touch.set_rotation(2);
            break;
        case LV_DISPLAY_ROTATION_270:
            touch.set_rotation(3);
            break;
        default:
            touch.set_rotation(0);
            break;
    }
}

/*---------------------------------------------------------------
 * Init principal
 *--------------------------------------------------------------*/
bool display_port_init(void)
{
    ESP_LOGI(TAG, "Init LCD");
    lcd.begin();

    ESP_LOGI(TAG, "Init touch");
    touch.begin();

    ESP_LOGI(TAG, "Init LVGL");
    lv_init();

    size_t buf_size = DRAW_BUF_PIXELS * sizeof(lv_color_t);

    ESP_LOGI(TAG, "Alloc draw buffers: %u bytes c/u", (unsigned)buf_size);

    s_buf1 = (lv_color_t *)heap_caps_malloc(
        buf_size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    s_buf2 = (lv_color_t *)heap_caps_malloc(
        buf_size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if(!s_buf1 || !s_buf2) {
        ESP_LOGE(TAG, "No se pudieron reservar draw buffers completos en PSRAM");
        if(s_buf1) {
            free(s_buf1);
            s_buf1 = nullptr;
        }
        if(s_buf2) {
            free(s_buf2);
            s_buf2 = nullptr;
        }
        return false;
    }

    s_display = lv_display_create(LCD_H_RES, LCD_V_RES);
    if(!s_display) {
        ESP_LOGE(TAG, "lv_display_create failed");
        free(s_buf1);
        free(s_buf2);
        s_buf1 = nullptr;
        s_buf2 = nullptr;
        return false;
    }

    lv_display_set_buffers(
        s_display,
        s_buf1,
        s_buf2,
        buf_size,
        LV_DISPLAY_RENDER_MODE_FULL
    );

    lv_display_set_flush_cb(s_display, my_disp_flush);
    lv_display_set_default(s_display);

    esp_lcd_dpi_panel_event_callbacks_t cbs = {};
    cbs.on_color_trans_done = lcd_color_trans_done_cb;

    esp_err_t err = esp_lcd_dpi_panel_register_event_callbacks(
        lcd.get_panel_handle(),
        &cbs,
        s_display
    );

    if(err != ESP_OK) {
        ESP_LOGW(TAG, "No se pudo registrar on_color_trans_done: %s", esp_err_to_name(err));
    }

    s_indev = lv_indev_create();
    if(!s_indev) {
        ESP_LOGE(TAG, "lv_indev_create failed");
        free(s_buf1);
        free(s_buf2);
        s_buf1 = nullptr;
        s_buf2 = nullptr;
        return false;
    }

    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, my_touchpad_read);

    display_set_rotation(LV_DISPLAY_ROTATION_0);

    app_ui_init();

    ESP_LOGI(TAG, "display_port_init OK");
    return true;
}

uint32_t display_port_lvgl_handler(void)
{
    return lv_timer_handler();
}

void display_port_set_flush_enabled(bool en)
{
    s_flush_enabled = en;
}

bool display_port_get_flush_enabled(void)
{
    return s_flush_enabled;
}