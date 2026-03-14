#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "bus_common.h"
#include "vfd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool vfd_link_init(void);
void vfd_link_task(void *arg);

bool vfd_link_ping(void);
bool vfd_link_read_info(void);
bool vfd_link_read_status(void);

bool vfd_link_start(void);
bool vfd_link_stop(void);
bool vfd_link_set_speed_kmh_x100(int32_t speed_x100);

bool vfd_link_is_online(void);
bool vfd_link_get_info(bus_info_payload_t *out_info);
bool vfd_link_get_status(vfd_status_t *out_status);

uint32_t vfd_link_get_ok_count(void);
uint32_t vfd_link_get_fail_count(void);

#ifdef __cplusplus
}
#endif