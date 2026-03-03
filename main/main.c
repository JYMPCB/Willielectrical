#include "board_config.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif


void app_main(void)
{
  // Inicialización de la app (ahora exclusivamente ESP-IDF)
  app_init();

  // Si tu app vive por tasks, no necesitás loop.
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

#ifdef __cplusplus
}
#endif