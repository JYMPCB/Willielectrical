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
#define EN_RS485   1
#define EN_SERVICES 0
#define EN_TI 0
#define EN_OTA 1

#if EN_DISPLAY
#include "display_port.h"
#endif
#if EN_AUDIO
#include "audio_mgr.h"
#endif
#if EN_TASKS
#include "app_globals.h"
#include "ui_refresh.h"
#include "tasks.h"
#include "app_state.h"
#include "app_logic.h"
#endif
#if EN_WIFI
#include "wifi_mgr.h"
#endif
#if EN_OTA
#include "ota_mgr.h"
#endif
#if EN_WIFI && EN_OTA
#include "mdns.h"
#endif
#if EN_RS485
#include "handlebar_link.h"
#include "vfd_link.h"
#endif
#if EN_SERVICES
#include "service_mgr.h"
#endif
#if EN_TI
#include "training_interval.h"
#endif

void app_init(void)
{
    static const char *TAG = "app";

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("wifi", ESP_LOG_VERBOSE);
    esp_log_level_set("esp_netif", ESP_LOG_VERBOSE);

#if EN_TASKS
    g_ui_mutex = xSemaphoreCreateMutex();
    if(g_ui_mutex == nullptr) {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex FAILED -> halt");
        while(1) vTaskDelay(pdMS_TO_TICKS(100));
    }
#endif

    ESP_LOGI(TAG, "Board: %s", BOARD_NAME);

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
    if(psram_bytes > 0) {
        ESP_LOGI(TAG, "PSRAM detectada y funcional");
        ESP_LOGI(TAG, "Tamaño total PSRAM: %u KB", (unsigned)(psram_bytes / 1024));
    } else {
        ESP_LOGI(TAG, "PSRAM NO detectada");
    }
    ESP_LOGI(TAG, "Heap PSRAM libre: %u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#else
    ESP_LOGI(TAG, "PSRAM NO detectada");
#endif

    ESP_LOGI(TAG, "Heap interno libre: %u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

#if EN_DISPLAY
    if(!display_port_init()) {
        ESP_LOGE(TAG, "display_port_init FAILED -> halt");
        while(1) vTaskDelay(pdMS_TO_TICKS(100));
    }

#endif

#if EN_AUDIO
    audio_init();
    audio_set_volume(86);
#endif

#if EN_TASKS
    app_state_init(); ESP_LOGI(TAG, "App core initialized");
    app_logic_init(); ESP_LOGI(TAG, "App logic initialized");
    ui_refresh_init(); ESP_LOGI(TAG, "UI refresh initialized");
    ui_refresh_start_timer(); ESP_LOGI(TAG, "UI refresh timer started");
    startTasks(); ESP_LOGI(TAG, "App core + UI refresh initialized");
    
#endif

#if EN_WIFI
    wifi_mgr_start_task();
#endif

#if EN_WIFI && EN_OTA
    auto ota_bootstrap_task = [](void *arg) {
        (void)arg;
        const uint32_t wait_step_ms = 200;

        while (!wifi_ok) {
            vTaskDelay(pdMS_TO_TICKS(wait_step_ms));
        }

        // mDNS: responde a willihmi.local en la red local
        mdns_init();
        mdns_hostname_set("willihmi");
        mdns_instance_name_set("Willi HMI");
        mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
        ESP_LOGI(TAG, "mDNS activo: willihmi.local:80");

        ota_local_start();
        ota_check_async();
        vTaskDelete(NULL);
    };

    BaseType_t ota_rc = xTaskCreatePinnedToCore(ota_bootstrap_task,
                                                 "ota_boot",
                                                 4096,
                                                 NULL,
                                                 2,
                                                 NULL,
                                                 0);
    if (ota_rc != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ota bootstrap task");
    }
#endif

#if EN_RS485
    if(handlebar_link_init()) {
        BaseType_t rc = xTaskCreatePinnedToCore(handlebar_link_task,
                                                "handlebar",
                                                4096,
                                                NULL,
                                                4,
                                                NULL,
                                                0);
        if(rc != pdPASS) {
            ESP_LOGE(TAG, "Failed to create handlebar task");
        }
    } else {
        ESP_LOGE(TAG, "handlebar_link_init failed (RS485 disabled)");
    }

    if(vfd_link_init()) {
        BaseType_t rc = xTaskCreatePinnedToCore(vfd_link_task,
                                                "vfd",
                                                4096,
                                                NULL,
                                                2,
                                                NULL,
                                                0);
        if(rc != pdPASS) {
            ESP_LOGE(TAG, "Failed to create vfd task");
        }
    } else {
        ESP_LOGE(TAG, "vfd_link_init failed (VFD disabled)");
    }
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