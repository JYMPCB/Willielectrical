#include "display_port.h"



#include "lv_conf.h"
#include "lvgl.h"
#include "esp_log.h"
#include "pins_config.h"
#include "jd9165_lcd.h"
#include "gt911_touch.h"
#include "ui.h"

static const char *TAG = "display_port";

// Mantengo exactamente tus objetos globales
static jd9165_lcd lcd = jd9165_lcd(LCD_RST);
static gt911_touch touch = gt911_touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf  = nullptr;
static lv_color_t *buf1 = nullptr;

static volatile bool s_flush_enabled = true;

// --- flush (igual a tu código) ---
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  if(!s_flush_enabled) {
    lv_disp_flush_ready(disp);
    return;
  }

  const int offsetx1 = area->x1;
  const int offsetx2 = area->x2;
  const int offsety1 = area->y1;
  const int offsety2 = area->y2;

  esp_err_t err = lcd.lcd_draw_bitmap(offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, (uint16_t*)color_p);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGW(TAG, "lcd_draw_bitmap failed: %s", esp_err_to_name(err));
  }

  lv_disp_flush_ready(disp);
}

// --- touch (igual a tu código, salvo que el Serial.printf lo dejo comentado por defecto) ---
static void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  (void)indev_driver;

  bool touched;
  uint16_t touchX, touchY;

  touched = touch.getTouch(&touchX, &touchY);

  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
    ESP_LOGD(TAG, "touch x=%u y=%u", touchX, touchY);
  }
}

// --- tu callback de rotación (en tu código estaba pero no se estaba registrando) ---
static void lvgl_port_update_callback(lv_disp_drv_t *drv)
{
  switch (drv->rotated) {
    case LV_DISP_ROT_NONE: touch.set_rotation(0); break;
    case LV_DISP_ROT_90:   touch.set_rotation(1); break;
    case LV_DISP_ROT_180:  touch.set_rotation(2); break;
    case LV_DISP_ROT_270:  touch.set_rotation(3); break;
  }
}

bool display_port_init(void)
{
  ESP_LOGI(TAG, "display_port_init()");

  lcd.begin();
  touch.begin();

  ESP_LOGI(TAG, "lv_init");
  lv_init();

  // Mantengo tu idea de full framebuffer. En tu código usabas sizeof(int32_t);
  // acá lo hago “correcto” por tipo (sin cambiar el resultado funcional).
  ESP_LOGI(TAG, "alloc buffers");
  size_t buf_pixels = LCD_H_RES * LCD_V_RES;
  size_t buffer_size = buf_pixels * sizeof(lv_color_t);
  buf  = (lv_color_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  buf1 = (lv_color_t*)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);

  /*const size_t buffer_size = sizeof(lv_color_t) * (size_t)LCD_H_RES * (size_t)LCD_V_RES;
  buf  = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
  buf1 = (lv_color_t *)heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);*/

  ESP_LOGI(TAG, "buf=%p buf1=%p", (void*)buf, (void*)buf1);
  if(!buf || !buf1) {
    ESP_LOGE(TAG, "No se pudo reservar PSRAM para buffers LVGL");
    return false;
  }

  ESP_LOGI(TAG, "draw_buf_init");
  lv_disp_draw_buf_init(&draw_buf, buf, buf1, (uint32_t)(LCD_H_RES * LCD_V_RES));

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_H_RES;
  disp_drv.ver_res = LCD_V_RES;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.full_refresh = false;

  // Registrar el callback de update (rotación)
  disp_drv.drv_update_cb = lvgl_port_update_callback;

  ESP_LOGI(TAG, "disp_drv_register");
  lv_disp_drv_register(&disp_drv);

  ESP_LOGI(TAG, "indev_register");
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
  
  ESP_LOGI(TAG, "display init done");
  ui_init();

  return true;
}

uint32_t display_port_lvgl_handler(void)
{
  return lv_timer_handler();
}

/*void display_port_loop(void)
{
  lv_timer_handler();
  delay(5);
}*/

void display_port_set_flush_enabled(bool en) { s_flush_enabled = en; }
bool display_port_get_flush_enabled(void) { return s_flush_enabled; }