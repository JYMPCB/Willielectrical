#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define START_BTN_PARTICLE_COUNT        18
#define START_BTN_SPRAY_PARTICLE_COUNT  16

typedef struct {
    lv_obj_t *start_btn;

    lv_obj_t *glow_outer;
    lv_obj_t *ring_outer;
    lv_obj_t *ring_inner;

    lv_obj_t *face;
    lv_obj_t *icon;
    lv_obj_t *title;
    lv_obj_t *line;
    lv_obj_t *subtitle;

    lv_obj_t *particles[START_BTN_PARTICLE_COUNT];
    lv_obj_t *spray_particles[START_BTN_SPRAY_PARTICLE_COUNT];

    bool pressed;
} willi_start_btn_t;

willi_start_btn_t *willi_start_btn_create(lv_obj_t *parent);

void willi_start_btn_set_title(willi_start_btn_t *btn, const char *txt);
void willi_start_btn_set_subtitle(willi_start_btn_t *btn, const char *txt);

void willi_start_btn_set_pressed(willi_start_btn_t *btn, bool pressed);

void willi_start_btn_create_particles(willi_start_btn_t *btn);
void willi_start_btn_create_spray_particles(willi_start_btn_t *btn);

void willi_start_btn_start_breathing(willi_start_btn_t *btn);
void willi_start_btn_start_particles_anim(willi_start_btn_t *btn);
void willi_start_btn_start_spray_anim(willi_start_btn_t *btn);

#ifdef __cplusplus
}
#endif