#include "audio_mgr.h"
#include "pins_config.h"


#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"

#include "driver/i2s_std.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "es8311.h"

static const char* TAG = "audio_mgr";
static constexpr uint32_t CODEC_I2C_SCL_HZ = 100000;
static constexpr float AUDIO_SW_GAIN = 1.6f;

#define AUDIO_DEBUG 0

#if AUDIO_DEBUG
#define AUDIO_DBGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define AUDIO_DBGW(...) ESP_LOGW(TAG, __VA_ARGS__)
#else
#define AUDIO_DBGI(...) do {} while(0)
#define AUDIO_DBGW(...) do {} while(0)
#endif

static QueueHandle_t s_q = nullptr;
static TaskHandle_t  s_task = nullptr;
static i2s_chan_handle_t s_tx = nullptr;
static i2c_master_bus_handle_t s_i2c_bus = nullptr;
#if AUDIO_DEBUG
static uint32_t s_evt_count = 0;
static uint32_t s_tone_count = 0;
#endif

static volatile uint8_t s_vol = 60; // 0..100
static volatile bool s_mute = false;
static volatile bool s_ok = false;

static uint8_t volume_to_es8311_reg(uint8_t vol_0_100)
{
  if (vol_0_100 > 100) vol_0_100 = 100;
  return (uint8_t)((vol_0_100 * 0xC0) / 100);
}

#ifndef ESP_RETURN_ON_ERROR
#define ESP_RETURN_ON_ERROR(x, tag, msg) do { \
  esp_err_t __e = (x);                        \
  if (__e != ESP_OK) {                        \
    ESP_LOGE((tag), "%s: %s", (msg), esp_err_to_name(__e)); \
    return __e;                               \
  }                                           \
} while(0)
#endif

static esp_err_t ensure_codec_i2c_bus()
{
  if (s_i2c_bus) return ESP_OK;

  esp_err_t err = i2c_master_get_bus_handle(I2C_NUM_0, &s_i2c_bus);
  if (err == ESP_OK) {
    return ESP_OK;
  }
  if (err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "i2c_master_get_bus_handle failed: %s", esp_err_to_name(err));
    return err;
  }

  i2c_master_bus_config_t bus_cfg = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = (gpio_num_t)TP_I2C_SDA,
    .scl_io_num = (gpio_num_t)TP_I2C_SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .intr_priority = 0,
    .trans_queue_depth = 0,
    .flags = {
      .enable_internal_pullup = true,
      .allow_pd = false,
    },
  };

  err = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

static esp_err_t i2s_init_like_example()
{
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan_cfg.auto_clear = true;

  ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx, nullptr), TAG, "i2s_new_channel");

  i2s_std_config_t std_cfg = {};
  std_cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;

  std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);

  std_cfg.gpio_cfg.mclk = (gpio_num_t)CODEC_I2S_MCLK;
  std_cfg.gpio_cfg.bclk = (gpio_num_t)CODEC_I2S_BCLK;
  std_cfg.gpio_cfg.ws   = (gpio_num_t)CODEC_I2S_LRCK;
  std_cfg.gpio_cfg.dout = (gpio_num_t)CODEC_I2S_DOUT;
  std_cfg.gpio_cfg.din  = I2S_GPIO_UNUSED;

  ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx, &std_cfg), TAG, "i2s_init_std");
  ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx), TAG, "i2s_enable");
  return ESP_OK;
}

static void write_tone(float freq_hz, int ms, float amp /*0..1*/)
{
  if(!s_ok || s_mute || !s_tx) {
    AUDIO_DBGW("write_tone skipped ok=%d mute=%d tx=%p", (int)s_ok, (int)s_mute, (void*)s_tx);
    return;
  }

#if AUDIO_DEBUG
  s_tone_count++;
  AUDIO_DBGI("tone #%lu freq=%.0f ms=%d amp=%.2f vol=%u", (unsigned long)s_tone_count, freq_hz, ms, amp, (unsigned)s_vol);
#endif

  const uint32_t sr = 16000;
  const int total = (int)((sr * (uint32_t)ms) / 1000UL);
  const float w = 2.0f * (float)M_PI * freq_hz / (float)sr;
  const int fade = (int)(sr * 3 / 1000); // 3ms

  int16_t buf[160 * 2]; // stereo interleaved
  for(int n=0; n<total; ){
    int frames = 160;
    if(n + frames > total) frames = total - n;

    for(int i=0;i<frames;i++, n++){
      float env = 1.0f;
      if(n < fade) env = (float)n / (float)fade;
      else if(total - n < fade) env = (float)(total - n) / (float)fade;

      float vol = ((float)s_vol / 100.0f) * AUDIO_SW_GAIN;
      if (vol > 1.0f) vol = 1.0f;
      float v = sinf(w * (float)n) * amp * env * vol;

      int32_t sample = (int32_t)(v * 32767.0f);
      if(sample > 32767) sample = 32767;
      if(sample < -32768) sample = -32768;

      int16_t s16 = (int16_t)sample;
      buf[i*2 + 0] = s16;
      buf[i*2 + 1] = s16;
    }

    size_t bw = 0;
    esp_err_t err = i2s_channel_write(s_tx, buf, frames * 2 * sizeof(int16_t), &bw, pdMS_TO_TICKS(200));
    if(err != ESP_OK){
      ESP_LOGE(TAG, "i2s write failed: %s", esp_err_to_name(err));
      return;
    }
    if (bw == 0) {
      AUDIO_DBGW("i2s write returned 0 bytes");
    }
  }
}

static void play_evt(audio_evt_t e)
{
  switch(e){
    case AUDIO_EVT_CLICK:
      write_tone(1800.0f, 28, 0.75f);
      break;
    case AUDIO_EVT_START:
      write_tone(1200.0f, 120, 0.45f);
      vTaskDelay(pdMS_TO_TICKS(40));
      write_tone(1600.0f, 120, 0.45f);
      break;
    case AUDIO_EVT_PAUSE:
      write_tone(900.0f, 180, 0.40f);
      break;
    case AUDIO_EVT_ERROR:
      for(int i=0;i<3;i++){
        write_tone(400.0f, 180, 0.50f);
        vTaskDelay(pdMS_TO_TICKS(80));
      }
      break;
  }
}

static void audio_task(void*)
{
  audio_evt_t evt;
  for(;;){
    if(xQueueReceive(s_q, &evt, portMAX_DELAY) == pdTRUE){
      AUDIO_DBGI("audio_task got evt=%d", (int)evt);
      play_evt(evt);
    }
  }
}

void audio_init()
{
  ESP_LOGI(TAG, "audio_init enter");

  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)PA_CTRL, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)PA_CTRL, 1));
  vTaskDelay(pdMS_TO_TICKS(200));

  esp_err_t err = ensure_codec_i2c_bus();
  if (err != ESP_OK) {
    s_ok = false;
    return;
  }

  // I2S como el ejemplo
  err = i2s_init_like_example();
  if(err != ESP_OK){
    ESP_LOGE(TAG, "i2s init failed: %s", esp_err_to_name(err));
    s_ok = false;
    return;
  }

  // Codec init (open/start + config_sample)
  es8311_cfg_t cc = {};
  cc.i2c_bus = s_i2c_bus;
  cc.i2c_addr = CODEC_I2C_ADDR;
  cc.i2c_scl_hz = CODEC_I2C_SCL_HZ;
  cc.master_mode = false;
  cc.use_mclk = true;
  cc.invert_mclk = false;
  cc.digital_mic = false;
  cc.mclk_div = 384; // clave: 16k * 384 = 6.144 MHz

  err = es8311_init_arduino(&cc, 16000);
  if(err != ESP_OK){
    ESP_LOGE(TAG, "es8311 init failed: %s", esp_err_to_name(err));
    s_ok = false;
    return;
  }

  // Queue/task
  if(!s_q) s_q = xQueueCreate(8, sizeof(audio_evt_t));
  if(!s_task) xTaskCreatePinnedToCore(audio_task, "audio", 4096, nullptr, 3, &s_task, 0);

  s_ok = true;
  ESP_LOGI(TAG, "audio_init ok");
}

void audio_play(audio_evt_t evt)
{
  if(!s_ok || !s_q) {
    AUDIO_DBGW("audio_play drop evt=%d ok=%d q=%p", (int)evt, (int)s_ok, (void*)s_q);
    return;
  }

#if AUDIO_DEBUG
  s_evt_count++;
#endif

  BaseType_t sent = xQueueSend(s_q, &evt, 0);
  if (sent != pdTRUE) {
    AUDIO_DBGW("audio_play queue full evt=%d count=%lu", (int)evt, (unsigned long)s_evt_count);
  } else {
    AUDIO_DBGI("audio_play enqueued evt=%d count=%lu", (int)evt, (unsigned long)s_evt_count);
  }
}

void audio_set_volume(uint8_t vol_0_100)
{
  if(vol_0_100 > 100) vol_0_100 = 100;
  s_vol = vol_0_100;
  if (s_ok) {
    esp_err_t err = es8311_set_volume_reg(volume_to_es8311_reg(vol_0_100));
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "es8311_set_volume_reg failed: %s", esp_err_to_name(err));
    }
  }
}

void audio_mute(bool mute)
{
  s_mute = mute;
  (void)es8311_set_mute(mute);
}