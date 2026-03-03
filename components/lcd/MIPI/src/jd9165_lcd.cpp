#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_ldo_regulator.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_lcd_jd9165.h"
#include "jd9165_lcd.h"

#define LCD_H_RES 1024
#define LCD_V_RES 600

#define MIPI_DPI_PX_FORMAT (LCD_COLOR_PIXEL_FORMAT_RGB565)
#define LCD_BIT_PER_PIXEL (16)

// “VDD_MIPI_DPHY”应供电 2.5V，可从内部 LDO 稳压器或外部 LDO 芯片获取电源
#define EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN 3 // LDO_VO3 连接至 VDD_MIPI_DPHY
#define EXAMPLE_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV 2500
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_BK_LIGHT GPIO_NUM_23

static bool chk(const char* tag, esp_err_t err) {
  if (err != ESP_OK) {
    ESP_LOGE("jd9165", "%s failed: %s (0x%X)", tag, esp_err_to_name(err), (unsigned)err);
    return false;
  }
  return true;
}

static void log_err(const char* tag, esp_err_t err) {
  ESP_LOGE("jd9165", "%s err=%s (0x%X)", tag, esp_err_to_name(err), (unsigned)err);
}

static const char *TAG = "example";
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;

jd9165_lcd::jd9165_lcd(int8_t lcd_rst)
  : _lcd_rst(lcd_rst),
    mipi_dsi_bus(nullptr),
    io_handle(nullptr),
    panel_handle(nullptr)
{
}

void jd9165_lcd::example_bsp_enable_dsi_phy_power()
{
    // 打开 MIPI DSI PHY 的电源，使其从“无电”状态进入“关机”状态
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
#ifdef EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = EXAMPLE_MIPI_DSI_PHY_PWR_LDO_CHAN,
        .voltage_mv = EXAMPLE_MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));
    ESP_LOGI(TAG, "MIPI DSI PHY Powered on");
#endif
}

void jd9165_lcd::example_bsp_init_lcd_backlight()
{
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT
        };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif
}

void jd9165_lcd::example_bsp_set_lcd_backlight(uint32_t level)
{
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, level);
#endif
}

void jd9165_lcd::begin()
{
  ESP_LOGI(TAG, "[jd9165] begin() start");

  example_bsp_enable_dsi_phy_power();
  example_bsp_init_lcd_backlight();
  example_bsp_set_lcd_backlight(EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL);

  ESP_LOGI(TAG, "[jd9165] new_dsi_bus...");
  esp_lcd_dsi_bus_config_t bus_config = JD9165_PANEL_BUS_DSI_2CH_CONFIG();

  esp_err_t err = esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);
  if (err != ESP_OK || !mipi_dsi_bus) {
    log_err("esp_lcd_new_dsi_bus", err);
    ESP_LOGE(TAG, "mipi_dsi_bus=%p", (void*)mipi_dsi_bus);
    while(1) vTaskDelay(pdMS_TO_TICKS(100));
  }
  ESP_LOGI(TAG, "[jd9165] mipi_dsi_bus=%p", (void*)mipi_dsi_bus);

  ESP_LOGI(TAG, "[jd9165] new_panel_io_dbi...");
  esp_lcd_dbi_io_config_t dbi_config = JD9165_PANEL_IO_DBI_CONFIG();

  err = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &io_handle);
  if (err != ESP_OK || !io_handle) {
    log_err("esp_lcd_new_panel_io_dbi", err);
    ESP_LOGE(TAG, "io_handle=%p", (void*)io_handle);
    while(1) vTaskDelay(pdMS_TO_TICKS(100));
  }
  ESP_LOGI(TAG, "[jd9165] io_handle=%p", (void*)io_handle);

  ESP_LOGI(TAG, "[jd9165] new_panel_jd9165...");
  dpi_config = JD9165_1024_600_PANEL_60HZ_DPI_CONFIG(MIPI_DPI_PX_FORMAT);

  vendor_config = {
    .mipi_config = {
      .dsi_bus = mipi_dsi_bus,
      .dpi_config = &dpi_config,
    },
  };

  panel_config = {
    .reset_gpio_num = _lcd_rst,
    .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = LCD_BIT_PER_PIXEL,
    .vendor_config  = &vendor_config,
  };

  err = esp_lcd_new_panel_jd9165(io_handle, &panel_config, &panel_handle);
  if (err != ESP_OK || !panel_handle) {
    log_err("esp_lcd_new_panel_jd9165", err);
    ESP_LOGE(TAG, "panel_handle=%p", (void*)panel_handle);
    while(1) vTaskDelay(pdMS_TO_TICKS(100));
  }
  ESP_LOGI(TAG, "[jd9165] panel_handle=%p", (void*)panel_handle);

  ESP_LOGI(TAG, "[jd9165] panel_reset...");
  err = esp_lcd_panel_reset(panel_handle);
  if (err != ESP_OK) {
    log_err("esp_lcd_panel_reset", err);
    while(1) vTaskDelay(pdMS_TO_TICKS(100));
  }

  ESP_LOGI(TAG, "[jd9165] panel_init...");
  err = esp_lcd_panel_init(panel_handle);
  if (err != ESP_OK) {
    log_err("esp_lcd_panel_init", err);
    while(1) vTaskDelay(pdMS_TO_TICKS(100));
  }

  example_bsp_set_lcd_backlight(EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
  ESP_LOGI(TAG, "[jd9165] begin() OK");
}

esp_err_t jd9165_lcd::lcd_draw_bitmap(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t *color_data)
{
  esp_err_t err = ESP_FAIL;

  for (int tries = 0; tries < 40; ++tries) {
    err = esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, color_data);
    if (err == ESP_OK) {
      return ESP_OK;
    }

    if (err != ESP_ERR_INVALID_STATE) {
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  return err;
}

void jd9165_lcd::draw16bitbergbbitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *color_data)
{
    uint16_t x_start = x;
    uint16_t y_start = y;
    uint16_t x_end = w + x;
    uint16_t y_end = h + y;

    (void)lcd_draw_bitmap(x_start, y_start, x_end, y_end, color_data);
}

void jd9165_lcd::fillScreen(uint16_t color)
{
    uint16_t *color_data = (uint16_t *)heap_caps_malloc(480 * 272 * 2, MALLOC_CAP_INTERNAL);
    memset(color_data, color, 480 * 272 * 2);
    draw16bitbergbbitmap(0, 0, 480, 272, color_data);
    free(color_data);
}

void jd9165_lcd::te_on()
{
    esp_lcd_panel_io_tx_param(io_handle, 0x35,new (uint8_t[]){0x00}, 1);
}

void jd9165_lcd::te_off()
{
    esp_lcd_panel_io_tx_param(io_handle, 0x34,new (uint8_t[]){0x00}, 0);
}

uint16_t jd9165_lcd::width()
{
    return LCD_H_RES;
}

uint16_t jd9165_lcd::height()
{
    return LCD_V_RES;
}
