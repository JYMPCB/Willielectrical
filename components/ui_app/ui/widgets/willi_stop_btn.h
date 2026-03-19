#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;
    lv_obj_t *face;
    lv_obj_t *icon_box;
    lv_obj_t *label;
    bool pressed;
} willi_stop_btn_t;

willi_stop_btn_t *willi_stop_btn_create(lv_obj_t *parent);
lv_obj_t *willi_stop_btn_get_root(willi_stop_btn_t *btn);
void willi_stop_btn_set_pressed(willi_stop_btn_t *btn, bool pressed);

#ifdef __cplusplus
}
#endif