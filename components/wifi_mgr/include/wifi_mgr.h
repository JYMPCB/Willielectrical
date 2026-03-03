#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_mgr_init(void);           // init netif + event loop + esp_wifi_init + start
void      wifi_mgr_start_task(void);     // crea la task que reemplaza al loop Arduino
void wifi_mgr_stop(void);
void wifi_mgr_on_enter_config(void);
void wifi_mgr_on_exit_config(void);

#ifdef __cplusplus
}
#endif