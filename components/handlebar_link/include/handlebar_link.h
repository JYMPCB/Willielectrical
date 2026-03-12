#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "handlebar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool handlebar_link_init(void);
void handlebar_link_task(void *arg);

bool handlebar_link_ping(void);
bool handlebar_link_read_info(void);
bool handlebar_link_read_status(void);
int  handlebar_link_read_events(bus_event_item_t *items, int max_items);
bool handlebar_link_set_mode(handlebar_mode_t mode);
bool handlebar_link_clear_events(void);

bool handlebar_link_is_online(void);
bool handlebar_link_get_info(bus_info_payload_t *out_info);
bool handlebar_link_get_status(handlebar_status_t *out_status);

uint32_t handlebar_link_get_ok_count(void);
uint32_t handlebar_link_get_fail_count(void);

#ifdef __cplusplus
}
#endif