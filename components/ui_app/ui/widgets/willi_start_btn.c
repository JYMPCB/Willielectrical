#include "../../../willi_common/include/willi_opacity.h"
#include "willi_start_btn.h"
#include <stdlib.h>

#define WILLI_GREEN         0x39FF14
#define WILLI_GREEN_SOFT    0x7CFF5B
#define WILLI_BG_1          0x121822
#define WILLI_BG_2          0x1D2430
#define WILLI_BORDER        0x5C6672
#define WILLI_TEXT_SUB      0xD6D6D6
#define WILLI_ORANGE        0xFFAA1D
#define WILLI_ORANGE_SOFT   0xFFCC66
#define WILLI_BLUE          0x35B7FF
#define WILLI_BLUE_SOFT     0x7FD4FF


typedef struct {
    int16_t x;
    int16_t y;
    uint8_t size;
    lv_opa_t opa;
    uint8_t glow;
} particle_cfg_t;

typedef struct {
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
    uint8_t size;
    lv_opa_t opa_min;
    lv_opa_t opa_max;
    uint8_t glow;
    uint16_t time;
    uint16_t delay;
} spray_particle_cfg_t;

typedef struct {
    lv_obj_t *obj;
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
} spray_anim_ctx_t;

/* ---------------------- partículas base ---------------------- */

static const particle_cfg_t particle_map[START_BTN_PARTICLE_COUNT] = {
    /* izquierda */
    {  34, 170,  4, OPA_30,  5 },
    {  24, 185,  6, OPA_45,  7 },
    {  12, 205,  8, OPA_70, 10 },
    {  22, 226,  5, OPA_45,  7 },
    {  38, 244,  4, OPA_28,  4 },
    {  52, 196,  3, OPA_22,  0 },
    {  58, 220,  3, OPA_20,  0 },
    {  48, 238,  2, OPA_18,  0 },

    /* derecha */
    { 392, 170,  4, OPA_30,  5 },
    { 402, 185,  6, OPA_45,  7 },
    { 414, 205,  8, OPA_70, 10 },
    { 404, 226,  5, OPA_45,  7 },
    { 388, 244,  4, OPA_28,  4 },
    { 374, 196,  3, OPA_22,  0 },
    { 368, 220,  3, OPA_20,  0 },
    { 378, 238,  2, OPA_18,  0 },

    /* dos extras */
    {  70, 210,  2, OPA_35,  0 },
    { 356, 210,  2, OPA_35,  0 }
};

/* ---------------------- partículas spray ---------------------- */

static const spray_particle_cfg_t spray_map[START_BTN_SPRAY_PARTICLE_COUNT] = {
    /* izquierda */
    {  86, 185,  58, 180, 3, OPA_8,  OPA_24,  4, 1200,    0 },
    {  84, 198,  44, 194, 2, OPA_6,  OPA_18,  0, 1350,  120 },
    {  82, 212,  34, 212, 4, OPA_8,  OPA_28,  5, 1250,  240 },
    {  84, 226,  48, 232, 3, OPA_6,  OPA_20,  4, 1400,  360 },
    {  88, 240,  62, 248, 2, OPA_6,  OPA_18,  0, 1300,  480 },
    {  92, 204,  68, 198, 2, OPA_6,  OPA_16,  0, 1450,  600 },
    {  90, 220,  54, 220, 3, OPA_8,  OPA_20,  3, 1200,  720 },
    {  94, 236,  70, 242, 2, OPA_6,  OPA_16,  0, 1500,  840 },

    /* derecha */
    { 344, 185, 372, 180, 3, OPA_8,  OPA_24,  4, 1200,   60 },
    { 346, 198, 386, 194, 2, OPA_6,  OPA_18,  0, 1350,  180 },
    { 348, 212, 396, 212, 4, OPA_8,  OPA_28,  5, 1250,  300 },
    { 346, 226, 382, 232, 3, OPA_6,  OPA_20,  4, 1400,  420 },
    { 342, 240, 368, 248, 2, OPA_6,  OPA_18,  0, 1300,  540 },
    { 338, 204, 362, 198, 2, OPA_6,  OPA_16,  0, 1450,  660 },
    { 340, 220, 376, 220, 3, OPA_8,  OPA_20,  3, 1200,  780 },
    { 336, 236, 360, 242, 2, OPA_6,  OPA_16,  0, 1500,  900 }
};

static spray_anim_ctx_t spray_ctx[START_BTN_SPRAY_PARTICLE_COUNT];

/* ---------------------- helpers ---------------------- */

static void set_circle_style(lv_obj_t *obj, lv_color_t bg1, lv_color_t bg2, lv_color_t border, lv_coord_t border_w)
{
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, bg1, 0);
    lv_obj_set_style_bg_grad_color(obj, bg2, 0);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_color(obj, border, 0);
    lv_obj_set_style_border_width(obj, border_w, 0);
}

static void style_transparent_circle(lv_obj_t *obj)
{
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, 0);
}

static void set_event_bubble_recursive(lv_obj_t *obj)
{
    if(!obj) return;

    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);

    uint32_t cnt = lv_obj_get_child_cnt(obj);
    for(uint32_t i = 0; i < cnt; i++) {
        lv_obj_t *child = lv_obj_get_child(obj, i);
        set_event_bubble_recursive(child);
    }
}

static lv_obj_t *create_particle(lv_obj_t *parent, const particle_cfg_t *cfg)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_remove_style_all(p);

    lv_obj_set_size(p, cfg->size, cfg->size);
    lv_obj_set_pos(p, cfg->x, cfg->y);

    lv_obj_set_style_radius(p, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(p, lv_color_hex(WILLI_GREEN), 0);
    lv_obj_set_style_bg_opa(p, cfg->opa, 0);
    lv_obj_set_style_border_width(p, 0, 0);

    if(cfg->glow > 0) {
        lv_obj_set_style_shadow_color(p, lv_color_hex(WILLI_GREEN), 0);
        lv_obj_set_style_shadow_width(p, cfg->glow, 0);
        lv_obj_set_style_shadow_opa(p, cfg->opa, 0);
    }

    return p;
}

static lv_obj_t *create_spray_particle(lv_obj_t *parent, const spray_particle_cfg_t *cfg)
{
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_remove_style_all(p);

    lv_obj_set_size(p, cfg->size, cfg->size);
    lv_obj_set_pos(p, cfg->x0, cfg->y0);

    lv_obj_set_style_radius(p, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(p, lv_color_hex(WILLI_GREEN), 0);
    lv_obj_set_style_bg_opa(p, cfg->opa_min, 0);
    lv_obj_set_style_border_width(p, 0, 0);

    if(cfg->glow > 0) {
        lv_obj_set_style_shadow_color(p, lv_color_hex(WILLI_GREEN), 0);
        lv_obj_set_style_shadow_width(p, cfg->glow, 0);
        lv_obj_set_style_shadow_opa(p, cfg->opa_max, 0);
    }

    return p;
}

static lv_color_t mode_main_color(willi_start_mode_t mode)
{
    switch(mode) {
        case WILLI_START_MODE_PROGRAM:  return lv_color_hex(WILLI_ORANGE);
        case WILLI_START_MODE_INTERVAL: return lv_color_hex(WILLI_BLUE);
        case WILLI_START_MODE_FREE:
        default:                        return lv_color_hex(WILLI_GREEN);
    }
}

static lv_color_t mode_soft_color(willi_start_mode_t mode)
{
    switch(mode) {
        case WILLI_START_MODE_PROGRAM:  return lv_color_hex(WILLI_ORANGE_SOFT);
        case WILLI_START_MODE_INTERVAL: return lv_color_hex(WILLI_BLUE_SOFT);
        case WILLI_START_MODE_FREE:
        default:                        return lv_color_hex(WILLI_GREEN_SOFT);
    }
}

static const char *mode_subtitle_text(willi_start_mode_t mode)
{
    switch(mode) {
        case WILLI_START_MODE_PROGRAM:  return "Programas";
        case WILLI_START_MODE_INTERVAL: return "Entrenamiento intervalado";
        case WILLI_START_MODE_FREE:
        default:                        return "Entrenamiento libre";
    }
}

static void willi_start_btn_apply_mode_visual(willi_start_btn_t *btn)
{
    if(!btn) return;

    lv_color_t c_main = mode_main_color(btn->mode);
    lv_color_t c_soft = mode_soft_color(btn->mode);

    if(btn->ring_outer) {
        lv_obj_set_style_border_color(btn->ring_outer, c_main, 0);
        lv_obj_set_style_shadow_color(btn->ring_outer, c_main, 0);
    }

    if(btn->ring_inner) {
        lv_obj_set_style_border_color(btn->ring_inner, c_soft, 0);
    }

    if(btn->glow_outer) {
        lv_obj_set_style_shadow_color(btn->glow_outer, c_main, 0);
    }

    if(btn->line) {
        lv_obj_set_style_bg_color(btn->line, c_main, 0);
        lv_obj_set_style_shadow_color(btn->line, c_main, 0);
    }

    if(btn->subtitle) {
        lv_label_set_text(btn->subtitle, mode_subtitle_text(btn->mode));
    }

    /* opcional: si luego querés cambiar también partículas por modo,
       acá es donde habría que hacerlo */
}

/* ---------------------- anim callbacks ---------------------- */

static void glow_width_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_width(obj, v, 0);
}

static void glow_opa_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_opa(obj, (lv_opa_t)v, 0);
}

static void ring_shadow_width_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_width(obj, v, 0);
}

static void ring_shadow_opa_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_opa(obj, (lv_opa_t)v, 0);
}

static void particle_opa_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)v, 0);
    lv_obj_set_style_shadow_opa(obj, (lv_opa_t)v, 0);
}

static void spray_pos_anim_cb(void *var, int32_t v)
{
    spray_anim_ctx_t *ctx = (spray_anim_ctx_t *)var;

    int32_t x = ctx->x0 + ((ctx->x1 - ctx->x0) * v) / 100;
    int32_t y = ctx->y0 + ((ctx->y1 - ctx->y0) * v) / 100;

    lv_obj_set_pos(ctx->obj, (lv_coord_t)x, (lv_coord_t)y);
}

static void spray_opa_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)v, 0);
    lv_obj_set_style_shadow_opa(obj, (lv_opa_t)v, 0);
}

static void press_anim_y_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_translate_y(obj, (lv_coord_t)v, 0);
}

static void face_shadow_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_width(obj, (lv_coord_t)v, 0);
}

static void face_zoom_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_transform_zoom(obj, (lv_coord_t)v, 0);
}

static void obj_shadow_width_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_width(obj, (lv_coord_t)v, 0);
}

static void obj_shadow_opa_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_shadow_opa(obj, (lv_opa_t)v, 0);
}

static void obj_width_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_width(obj, (lv_coord_t)v);
}

static void obj_bg_opa_anim_cb(void *var, int32_t v)
{
    lv_obj_t *obj = (lv_obj_t *)var;
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)v, 0);
}

/* ---------------------- events ---------------------- */

static void event_cb(lv_event_t *e)
{
    willi_start_btn_t *btn = (willi_start_btn_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED) {
        willi_start_btn_set_pressed(btn, true);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        willi_start_btn_set_pressed(btn, false);
    }
}

/* ---------------------- public create parts ---------------------- */

void willi_start_btn_create_particles(willi_start_btn_t *btn)
{
    if(!btn) return;

    for(int i = 0; i < START_BTN_PARTICLE_COUNT; i++) {
        btn->particles[i] = create_particle(btn->start_btn, &particle_map[i]);
    }
}

void willi_start_btn_create_spray_particles(willi_start_btn_t *btn)
{
    if(!btn) return;

    for(int i = 0; i < START_BTN_SPRAY_PARTICLE_COUNT; i++) {
        btn->spray_particles[i] = create_spray_particle(btn->start_btn, &spray_map[i]);
    }
}

/* ---------------------- main create ---------------------- */

willi_start_btn_t *willi_start_btn_create(lv_obj_t *parent)
{
    willi_start_btn_t *btn = (willi_start_btn_t *)malloc(sizeof(willi_start_btn_t));
    if(!btn) return NULL;

    btn->pressed = false;
    btn->mode = WILLI_START_MODE_FREE;

    for(int i = 0; i < START_BTN_PARTICLE_COUNT; i++) btn->particles[i] = NULL;
    for(int i = 0; i < START_BTN_SPRAY_PARTICLE_COUNT; i++) btn->spray_particles[i] = NULL;

    btn->start_btn = lv_obj_create(parent);
    lv_obj_remove_style_all(btn->start_btn);
    lv_obj_set_size(btn->start_btn, 430, 430);
    lv_obj_center(btn->start_btn);
    lv_obj_clear_flag(btn->start_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn->start_btn, LV_OBJ_FLAG_CLICKABLE);
#ifdef LV_OBJ_FLAG_OVERFLOW_VISIBLE
    lv_obj_add_flag(btn->start_btn, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
#endif

    btn->glow_outer = lv_obj_create(btn->start_btn);
    lv_obj_remove_style_all(btn->glow_outer);
    lv_obj_set_size(btn->glow_outer, 356, 356);
    lv_obj_center(btn->glow_outer);
    style_transparent_circle(btn->glow_outer);
    lv_obj_set_style_shadow_color(btn->glow_outer, lv_color_hex(WILLI_GREEN), 0);
    lv_obj_set_style_shadow_width(btn->glow_outer, 26, 0);
    lv_obj_set_style_shadow_spread(btn->glow_outer, 0, 0);
    lv_obj_set_style_shadow_opa(btn->glow_outer, OPA_50, 0);

    btn->ring_outer = lv_obj_create(btn->start_btn);
    lv_obj_remove_style_all(btn->ring_outer);
    lv_obj_set_size(btn->ring_outer, 340, 340);
    lv_obj_center(btn->ring_outer);
    style_transparent_circle(btn->ring_outer);
    lv_obj_set_style_border_width(btn->ring_outer, 6, 0);
    lv_obj_set_style_border_color(btn->ring_outer, lv_color_hex(WILLI_GREEN), 0);
    lv_obj_set_style_border_opa(btn->ring_outer, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(btn->ring_outer, lv_color_hex(WILLI_GREEN), 0);
    lv_obj_set_style_shadow_width(btn->ring_outer, 18, 0);
    lv_obj_set_style_shadow_opa(btn->ring_outer, OPA_60, 0);

    btn->ring_inner = lv_obj_create(btn->start_btn);
    lv_obj_remove_style_all(btn->ring_inner);
    lv_obj_set_size(btn->ring_inner, 314, 314);
    lv_obj_center(btn->ring_inner);
    style_transparent_circle(btn->ring_inner);
    lv_obj_set_style_border_width(btn->ring_inner, 2, 0);
    lv_obj_set_style_border_color(btn->ring_inner, lv_color_hex(WILLI_GREEN_SOFT), 0);
    lv_obj_set_style_border_opa(btn->ring_inner, OPA_50, 0);

    btn->face = lv_obj_create(btn->start_btn);
    lv_obj_remove_style_all(btn->face);
    lv_obj_set_size(btn->face, 286, 286);
    lv_obj_center(btn->face);
    set_circle_style(
        btn->face,
        lv_color_hex(WILLI_BG_1),
        lv_color_hex(WILLI_BG_2),
        lv_color_hex(WILLI_BORDER),
        2
    );
    lv_obj_set_style_shadow_color(btn->face, lv_color_black(), 0);
    lv_obj_set_style_shadow_width(btn->face, 20, 0);
    lv_obj_set_style_shadow_opa(btn->face, OPA_35, 0);
    lv_obj_set_style_pad_all(btn->face, 0, 0);

    lv_obj_set_style_transform_pivot_x(btn->face, 143, 0);
    lv_obj_set_style_transform_pivot_y(btn->face, 143, 0);
    lv_obj_set_style_transform_zoom(btn->face, 256, 0);

    btn->icon = lv_label_create(btn->face);
    lv_label_set_text(btn->icon, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(btn->icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn->icon, &lv_font_montserrat_48, 0);
    lv_obj_align(btn->icon, LV_ALIGN_CENTER, 0, -70);

    btn->title = lv_label_create(btn->face);
    lv_label_set_text(btn->title, "COMENZAR");
    lv_obj_set_style_text_color(btn->title, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn->title, &lv_font_montserrat_34, 0);
    lv_obj_align(btn->title, LV_ALIGN_CENTER, 0, 6);

    btn->line = lv_obj_create(btn->face);
    lv_obj_remove_style_all(btn->line);
    lv_obj_set_size(btn->line, 138, 2);
    lv_obj_set_style_bg_color(btn->line, lv_color_hex(WILLI_GREEN), 0);
    lv_obj_set_style_bg_opa(btn->line, OPA_90, 0);
    lv_obj_set_style_shadow_color(btn->line, lv_color_hex(WILLI_GREEN), 0);
    lv_obj_set_style_shadow_width(btn->line, 8, 0);
    lv_obj_set_style_shadow_opa(btn->line, OPA_50, 0);
    lv_obj_align(btn->line, LV_ALIGN_CENTER, 0, 44);

    btn->subtitle = lv_label_create(btn->face);
    lv_label_set_text(btn->subtitle, "Entrenamiento libre");
    lv_obj_set_style_text_color(btn->subtitle, lv_color_hex(WILLI_TEXT_SUB), 0);
    lv_obj_set_style_text_font(btn->subtitle, &lv_font_montserrat_20, 0);
    lv_obj_align(btn->subtitle, LV_ALIGN_CENTER, 0, 72);

    willi_start_btn_create_particles(btn);
    willi_start_btn_create_spray_particles(btn);

    for(int i = 0; i < START_BTN_PARTICLE_COUNT; i++) {
        if(btn->particles[i]) lv_obj_move_foreground(btn->particles[i]);
    }

    for(int i = 0; i < START_BTN_SPRAY_PARTICLE_COUNT; i++) {
        if(btn->spray_particles[i]) lv_obj_move_foreground(btn->spray_particles[i]);
    }

    lv_obj_move_foreground(btn->glow_outer);
    lv_obj_move_foreground(btn->ring_outer);
    lv_obj_move_foreground(btn->ring_inner);
    lv_obj_move_foreground(btn->face);

    set_event_bubble_recursive(btn->start_btn);
    lv_obj_add_event_cb(btn->start_btn, event_cb, LV_EVENT_ALL, btn);

    willi_start_btn_apply_mode_visual(btn);
    return btn;
}

/* ---------------------- setters ---------------------- */

void willi_start_btn_set_title(willi_start_btn_t *btn, const char *txt)
{
    if(!btn || !btn->title) return;
    lv_label_set_text(btn->title, txt);
}

void willi_start_btn_set_subtitle(willi_start_btn_t *btn, const char *txt)
{
    if(!btn || !btn->subtitle) return;
    lv_label_set_text(btn->subtitle, txt);
}

void willi_start_btn_set_mode(willi_start_btn_t *btn, willi_start_mode_t mode)
{
    if(!btn) return;
    if(btn->mode == mode) return;

    btn->mode = mode;
    willi_start_btn_apply_mode_visual(btn);
}

willi_start_mode_t willi_start_btn_get_mode(willi_start_btn_t *btn)
{
    if(!btn) return WILLI_START_MODE_FREE;
    return btn->mode;
}
/* ---------------------- press effect ---------------------- */

void willi_start_btn_set_pressed(willi_start_btn_t *btn, bool pressed)
{
    if(!btn) return;
    if(btn->pressed == pressed) return;

    btn->pressed = pressed;

    lv_anim_del(btn->face, press_anim_y_cb);
    lv_anim_del(btn->face, face_shadow_anim_cb);
    lv_anim_del(btn->face, face_zoom_anim_cb);

    lv_anim_del(btn->ring_outer, obj_shadow_width_anim_cb);
    lv_anim_del(btn->ring_outer, obj_shadow_opa_anim_cb);

    lv_anim_del(btn->glow_outer, obj_shadow_width_anim_cb);
    lv_anim_del(btn->glow_outer, obj_shadow_opa_anim_cb);

    lv_anim_del(btn->line, obj_width_anim_cb);
    lv_anim_del(btn->line, obj_bg_opa_anim_cb);
    lv_anim_del(btn->line, obj_shadow_opa_anim_cb);

    lv_anim_t a;

    if(pressed) {
        /* face baja apenas */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 0, 4);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, press_anim_y_cb);
        lv_anim_start(&a);

        /* compresión sutil */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 256, 248);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, face_zoom_anim_cb);
        lv_anim_start(&a);

        /* sombra más corta */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 20, 10);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, face_shadow_anim_cb);
        lv_anim_start(&a);

        /* aro externo se contrae */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->ring_outer);
        lv_anim_set_values(&a, 18, 12);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_width_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->ring_outer);
        lv_anim_set_values(&a, OPA_60, OPA_38);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_opa_anim_cb);
        lv_anim_start(&a);

        /* glow general baja */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->glow_outer);
        lv_anim_set_values(&a, 26, 16);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_width_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->glow_outer);
        lv_anim_set_values(&a, OPA_50, OPA_28);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_opa_anim_cb);
        lv_anim_start(&a);

        /* línea verde se comprime un poco */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->line);
        lv_anim_set_values(&a, 138, 112);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_width_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->line);
        lv_anim_set_values(&a, OPA_90, OPA_60);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_bg_opa_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->line);
        lv_anim_set_values(&a, OPA_50, OPA_24);
        lv_anim_set_time(&a, 95);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_opa_anim_cb);
        lv_anim_start(&a);

        /* contenido acompaña menos */
        lv_obj_set_style_translate_y(btn->icon, 2, 0);
        lv_obj_set_style_translate_y(btn->title, 2, 0);
        lv_obj_set_style_translate_y(btn->line, 1, 0);
        lv_obj_set_style_translate_y(btn->subtitle, 1, 0);
    } else {
        /* vuelve con soltura */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 4, 0);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&a, press_anim_y_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 248, 256);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&a, face_zoom_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->face);
        lv_anim_set_values(&a, 10, 20);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, face_shadow_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->ring_outer);
        lv_anim_set_values(&a, 12, 18);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_width_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->ring_outer);
        lv_anim_set_values(&a, OPA_38, OPA_60);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_opa_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->glow_outer);
        lv_anim_set_values(&a, 16, 26);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_width_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->glow_outer);
        lv_anim_set_values(&a, OPA_28, OPA_50);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_opa_anim_cb);
        lv_anim_start(&a);

        /* línea vuelve */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->line);
        lv_anim_set_values(&a, 112, 138);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_width_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->line);
        lv_anim_set_values(&a, OPA_60, OPA_90);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_bg_opa_anim_cb);
        lv_anim_start(&a);

        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->line);
        lv_anim_set_values(&a, OPA_24, OPA_50);
        lv_anim_set_time(&a, 165);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, obj_shadow_opa_anim_cb);
        lv_anim_start(&a);

        lv_obj_set_style_translate_y(btn->icon, 0, 0);
        lv_obj_set_style_translate_y(btn->title, 0, 0);
        lv_obj_set_style_translate_y(btn->line, 0, 0);
        lv_obj_set_style_translate_y(btn->subtitle, 0, 0);
    }
}

/* ---------------------- anim start ---------------------- */

void willi_start_btn_start_breathing(willi_start_btn_t *btn)
{
    if(!btn) return;

    lv_anim_del(btn->glow_outer, glow_width_anim_cb);
    lv_anim_del(btn->glow_outer, glow_opa_anim_cb);
    lv_anim_del(btn->ring_outer, ring_shadow_width_anim_cb);
    lv_anim_del(btn->ring_outer, ring_shadow_opa_anim_cb);

    lv_anim_t a;

    /* glow externo */
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn->glow_outer);
    lv_anim_set_values(&a, 22, 34);
    lv_anim_set_time(&a, 1500);
    lv_anim_set_playback_time(&a, 1500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, glow_width_anim_cb);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, btn->glow_outer);
    lv_anim_set_values(&a, OPA_38, OPA_60);
    lv_anim_set_time(&a, 1500);
    lv_anim_set_playback_time(&a, 1500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, glow_opa_anim_cb);
    lv_anim_start(&a);

    /* aro externo acompaña un poco */
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn->ring_outer);
    lv_anim_set_values(&a, 14, 22);
    lv_anim_set_time(&a, 1500);
    lv_anim_set_playback_time(&a, 1500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, ring_shadow_width_anim_cb);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, btn->ring_outer);
    lv_anim_set_values(&a, OPA_40, OPA_70);
    lv_anim_set_time(&a, 1500);
    lv_anim_set_playback_time(&a, 1500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, ring_shadow_opa_anim_cb);
    lv_anim_start(&a);
}

void willi_start_btn_start_particles_anim(willi_start_btn_t *btn)
{
    if(!btn) return;

    for(int i = 0; i < START_BTN_PARTICLE_COUNT; i++) {
        if(!btn->particles[i]) continue;

        lv_anim_del(btn->particles[i], particle_opa_anim_cb);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->particles[i]);

        lv_opa_t base = particle_map[i].opa;
        lv_opa_t min = (base > 16) ? (lv_opa_t)(base - 14) : (lv_opa_t)8;
        lv_opa_t max = (base < 240) ? (lv_opa_t)(base + 14) : (lv_opa_t)255;

        lv_anim_set_values(&a, min, max);
        lv_anim_set_time(&a, 1000 + (i * 50));
        lv_anim_set_playback_time(&a, 1000 + (i * 50));
        lv_anim_set_delay(&a, i * 35);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_set_exec_cb(&a, particle_opa_anim_cb);
        lv_anim_start(&a);
    }
}

void willi_start_btn_start_spray_anim(willi_start_btn_t *btn)
{
    if(!btn) return;

    for(int i = 0; i < START_BTN_SPRAY_PARTICLE_COUNT; i++) {
        if(!btn->spray_particles[i]) continue;

        spray_ctx[i].obj = btn->spray_particles[i];
        spray_ctx[i].x0 = spray_map[i].x0;
        spray_ctx[i].y0 = spray_map[i].y0;
        spray_ctx[i].x1 = spray_map[i].x1;
        spray_ctx[i].y1 = spray_map[i].y1;

        lv_anim_del(&spray_ctx[i], spray_pos_anim_cb);
        lv_anim_del(btn->spray_particles[i], spray_opa_anim_cb);

        lv_anim_t a;

        /* movimiento suave y lineal */
        lv_anim_init(&a);
        lv_anim_set_var(&a, &spray_ctx[i]);
        lv_anim_set_values(&a, 0, 100);
        lv_anim_set_time(&a, spray_map[i].time);
        lv_anim_set_delay(&a, spray_map[i].delay);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&a, lv_anim_path_linear);
        lv_anim_set_exec_cb(&a, spray_pos_anim_cb);
        lv_anim_start(&a);

        /* opacidad sube y baja, así cuando reinicia posición casi no se nota */
        lv_anim_init(&a);
        lv_anim_set_var(&a, btn->spray_particles[i]);
        lv_anim_set_values(&a, spray_map[i].opa_min, spray_map[i].opa_max);
        lv_anim_set_time(&a, spray_map[i].time / 2);
        lv_anim_set_playback_time(&a, spray_map[i].time / 2);
        lv_anim_set_delay(&a, spray_map[i].delay);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
        lv_anim_set_exec_cb(&a, spray_opa_anim_cb);
        lv_anim_start(&a);
    }
}