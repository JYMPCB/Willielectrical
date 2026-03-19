#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WILLI_STEPPER_ROLE_SPEED = 0,
    WILLI_STEPPER_ROLE_PACE
} willi_stepper_role_t;

typedef struct {
    lv_obj_t *root;

    lv_obj_t *minus_zone;
    lv_obj_t *center_zone;
    lv_obj_t *plus_zone;

    lv_obj_t *minus_label;
    lv_obj_t *title_label;
    lv_obj_t *plus_label;

    lv_color_t accent;
    willi_stepper_role_t role;

    bool minus_pressed;
    bool plus_pressed;
} willi_stepper_btn_t;

willi_stepper_btn_t *willi_stepper_btn_create(lv_obj_t *parent);

void willi_stepper_btn_set_title(willi_stepper_btn_t *btn, const char *txt);
void willi_stepper_btn_set_accent(willi_stepper_btn_t *btn, lv_color_t color);
void willi_stepper_btn_set_role(willi_stepper_btn_t *btn, willi_stepper_role_t role);

void willi_stepper_btn_set_minus_pressed(willi_stepper_btn_t *btn, bool pressed);
void willi_stepper_btn_set_plus_pressed(willi_stepper_btn_t *btn, bool pressed);

lv_obj_t *willi_stepper_btn_get_root(willi_stepper_btn_t *btn);
lv_obj_t *willi_stepper_btn_get_minus_zone(willi_stepper_btn_t *btn);
lv_obj_t *willi_stepper_btn_get_plus_zone(willi_stepper_btn_t *btn);

#ifdef __cplusplus
}
#endif