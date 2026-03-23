#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool display_port_init(void);
void display_port_loop(void);

// Opcional (por si luego querés congelar flush, OTA, etc.)
void display_port_set_flush_enabled(bool en);
bool display_port_get_flush_enabled(void);

uint32_t display_port_lvgl_handler(void);

#ifdef __cplusplus
}
#endif