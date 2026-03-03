#ifdef __cplusplus
extern "C" {
#endif
#include "board_config.h"
#include "app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_private/esp_clk.h"

#define EN_DISPLAY 1
#define EN_AUDIO   1
#define EN_TASKS   1
#define EN_WIFI    1
#define EN_SERVICES 1
#define EN_TI 1

#if EN_DISPLAY
#include "display_port.h"
#endif
#if EN_AUDIO
#include "audio_mgr.h"
#endif
#include "ui_refresh.h"
#if EN_TASKS
#include "app_globals.h"
#include "tasks.h"  
#endif
#if EN_WIFI
#include "wifi_mgr.h"
#endif
#if EN_SERVICES
#include "service_mgr.h"
#endif
#if EN_TI
#include "training_interval.h"
#endif

void app_init()
{
  static const char* TAG = "app";
  esp_log_level_set("*", ESP_LOG_INFO);
  esp_log_level_set("wifi", ESP_LOG_VERBOSE);
  esp_log_level_set("esp_netif", ESP_LOG_VERBOSE);
  // esp_log_level_set("tcpip_adapter", ESP_LOG_VERBOSE); // <- fuera

  ESP_LOGI(TAG, "Board: %s", BOARD_NAME);

  // Info de chip usando ESP-IDF
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  uint32_t cpu_freq_mhz = (uint32_t)(esp_clk_cpu_freq() / 1000000);
    ESP_LOGI(TAG, "CPU: %d, rev %d, CPU Freq: %d MHz, %d core(s)",
         chip_info.model, chip_info.revision, cpu_freq_mhz, chip_info.cores);
    ESP_LOGI(TAG, "Free heap: %d bytes", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
#ifdef MALLOC_CAP_SPIRAM
  ESP_LOGI(TAG, "Free PSRAM size: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#endif
  ESP_LOGI(TAG, "SDK version: %s", esp_get_idf_version());

#ifdef MALLOC_CAP_SPIRAM
  size_t psram_bytes = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  if (psram_bytes > 0) {
    ESP_LOGI(TAG, "✅ PSRAM detectada y funcional");
    char buf[64];
    snprintf(buf, sizeof(buf), "Tamaño total PSRAM: %u KB", (unsigned)(psram_bytes / 1024));
    ESP_LOGI(TAG, "%s", buf);
  } else {
    ESP_LOGI(TAG, "❌ PSRAM NO detectada");
  }
  char buf2[64];
  snprintf(buf2, sizeof(buf2), "Heap PSRAM libre: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  ESP_LOGI(TAG, "%s", buf2);
#else
  ESP_LOGI(TAG, "❌ PSRAM NO detectada");
#endif
  char buf3[64];
  snprintf(buf3, sizeof(buf3), "Heap interno libre: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
  ESP_LOGI(TAG, "%s", buf3);

  #if EN_DISPLAY
  if(!display_port_init()){
    ESP_LOGE(TAG, "display_port_init FAILED -> halt");
    while(1) vTaskDelay(pdMS_TO_TICKS(100));
  }
  #endif

  #if EN_AUDIO
  audio_init();
  audio_set_volume(95);
  #endif

  #if EN_TASKS
  g_ui_mutex = xSemaphoreCreateMutex();
  startTasks();
  #endif

  ui_refresh_start_timer();

  #if EN_WIFI
  ESP_ERROR_CHECK(wifi_mgr_init());
  wifi_mgr_start_task();
  #endif

  #if EN_TI
  ti_init();
  #endif

  #if EN_SERVICES
  g_service.begin();
  #endif
}

#ifdef __cplusplus
}
#endif