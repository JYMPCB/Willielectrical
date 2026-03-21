#include "willi_training_fx.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void set_hidden(lv_obj_t *obj, bool hidden)
{
    if(!obj) return;
    if(hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void set_hidden_obj_array(lv_obj_t **arr, uint16_t count, bool hidden)
{
    if(!arr) return;

    for(uint16_t i = 0; i < count; i++) {
        if(arr[i]) set_hidden(arr[i], hidden);
    }
}

static void move_obj_array_fg(lv_obj_t **arr, uint16_t count)
{
    if(!arr) return;

    for(uint16_t i = 0; i < count; i++) {
        if(arr[i]) lv_obj_move_foreground(arr[i]);
    }
}

static void style_travel_dot(lv_obj_t *obj, lv_color_t color, lv_coord_t size, lv_opa_t opa)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_set_style_shadow_width(obj, 32, 0);
    lv_obj_set_style_shadow_spread(obj, 2, 0);
    lv_obj_set_style_shadow_color(obj, color, 0);
    lv_obj_set_style_shadow_opa(obj, opa, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void style_halo_dot(lv_obj_t *obj, lv_color_t color, lv_coord_t size, lv_opa_t opa)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void style_trail_particle(lv_obj_t *obj, lv_color_t color, lv_coord_t size, lv_opa_t opa)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_set_style_shadow_width(obj, size * 2, 0);
    lv_obj_set_style_shadow_color(obj, color, 0);
    lv_obj_set_style_shadow_opa(obj, opa, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

/* ---------- curva ---------- */

static void build_u_wave(lv_point_t *pts,
                         uint16_t count,
                         lv_coord_t x_left,
                         lv_coord_t x_right,
                         lv_coord_t y_base,
                         lv_coord_t depth)
{
    for(uint16_t i = 0; i < count; i++) {
        float t = (float)i / (float)(count - 1);
        float s = sinf(t * (float)M_PI);
        float y = (float)y_base + s * (float)depth;

        pts[i].x = x_left + (lv_coord_t)((float)(x_right - x_left) * t);
        pts[i].y = (lv_coord_t)y;
    }
}

static lv_point_t interp_point(const lv_point_t *pts, uint16_t count, float pos)
{
    lv_point_t out = {0, 0};

    int idx = (int)pos;
    float frac = pos - (float)idx;

    int next = idx + 1;
    if(next >= count) next = count - 1;

    out.x = pts[idx].x + (lv_coord_t)((pts[next].x - pts[idx].x) * frac);
    out.y = pts[idx].y + (lv_coord_t)((pts[next].y - pts[idx].y) * frac);

    return out;
}

/* ---------- partículas ---------- */

static void create_trail_particles(lv_obj_t *parent, lv_obj_t **arr, uint16_t count, lv_color_t color)
{
    for(uint16_t i = 0; i < count; i++) {

        lv_coord_t size;
        lv_opa_t opa;

        if(i == 0) {
            size = 6; opa = LV_OPA_40;
        } else if(i < 3) {
            size = 5; opa = LV_OPA_30;
        } else if(i < 5) {
            size = 4; opa = LV_OPA_20;
        } else {
            size = 3; opa = LV_OPA_10;
        }

        arr[i] = lv_obj_create(parent);
        style_trail_particle(arr[i], color, size, opa);
    }
}

static void place_trail_particles(lv_obj_t **arr,
                                  uint16_t count,
                                  const lv_point_t *pts,
                                  uint16_t pts_count,
                                  float head_pos,
                                  float spacing,
                                  lv_coord_t base_radius)
{
    for(uint16_t i = 0; i < count; i++) {

        float pos = head_pos - ((float)(i + 1) * spacing);

        while(pos < 0.0f)
            pos += (float)(pts_count - 1);

        lv_point_t p = interp_point(pts, pts_count, pos);

        lv_coord_t r = (i == 0) ? base_radius : base_radius - 1;
        if(r < 1) r = 1;

        lv_obj_set_pos(arr[i], p.x - r, p.y - r);
    }
}

/* ---------- TIMER ---------- */

static void fx_timer_cb(lv_timer_t *t)
{
    willi_training_fx_t *fx = (willi_training_fx_t *)lv_timer_get_user_data(t);
    if(!fx) return;

    fx->travel_pos_green += 0.28f;
    fx->travel_pos_blue  += 0.18f;

    if(fx->travel_pos_green > (float)(WILLI_WAVE_PTS - 1)) fx->travel_pos_green = 0;
    if(fx->travel_pos_blue  > (float)(WILLI_WAVE_PTS - 1)) fx->travel_pos_blue  = 0;

    lv_point_t pg = interp_point(fx->wave_green_pts, WILLI_WAVE_PTS, fx->travel_pos_green);
    lv_point_t pb = interp_point(fx->wave_blue_pts,  WILLI_WAVE_PTS, fx->travel_pos_blue);

    lv_obj_set_pos(fx->travel_dot_green, pg.x - 4, pg.y - 4);
    lv_obj_set_pos(fx->travel_dot_blue,  pb.x - 3, pb.y - 3);

    place_trail_particles(fx->trail_green, WILLI_TRAIL_COUNT,
                          fx->wave_green_pts, WILLI_WAVE_PTS,
                          fx->travel_pos_green, 1.3f, 3);

    place_trail_particles(fx->trail_blue, WILLI_TRAIL_COUNT,
                          fx->wave_blue_pts, WILLI_WAVE_PTS,
                          fx->travel_pos_blue, 1.1f, 2);
}

/* ---------- PUBLIC ---------- */

void willi_training_fx_create(lv_obj_t *parent,
                              willi_training_fx_t *fx,
                              lv_coord_t x_left,
                              lv_coord_t x_right,
                              lv_coord_t green_base_y,
                              lv_coord_t green_depth,
                              lv_coord_t blue_base_y,
                              lv_coord_t blue_depth)
{
    build_u_wave(fx->wave_green_pts, WILLI_WAVE_PTS, x_left, x_right, green_base_y, green_depth);
    build_u_wave(fx->wave_blue_pts,  WILLI_WAVE_PTS, x_left + 52, x_right - 56, blue_base_y, blue_depth);

    create_trail_particles(parent, fx->trail_green, WILLI_TRAIL_COUNT, lv_color_hex(0x7CFF72));
    create_trail_particles(parent, fx->trail_blue,  WILLI_TRAIL_COUNT, lv_color_hex(0x37D8FF));

    fx->travel_dot_green = lv_obj_create(parent);
    style_travel_dot(fx->travel_dot_green, lv_color_hex(0x8AFF72), 8, LV_OPA_90);

    fx->travel_dot_blue = lv_obj_create(parent);
    style_travel_dot(fx->travel_dot_blue, lv_color_hex(0x37D8FF), 6, LV_OPA_80);

    move_obj_array_fg(fx->trail_green, WILLI_TRAIL_COUNT);
    move_obj_array_fg(fx->trail_blue, WILLI_TRAIL_COUNT);

    lv_obj_move_foreground(fx->travel_dot_green);
    lv_obj_move_foreground(fx->travel_dot_blue);
}

void willi_training_fx_show(willi_training_fx_t *fx)
{
    set_hidden_obj_array(fx->trail_green, WILLI_TRAIL_COUNT, false);
    set_hidden_obj_array(fx->trail_blue, WILLI_TRAIL_COUNT, false);
    set_hidden(fx->travel_dot_green, false);
    set_hidden(fx->travel_dot_blue, false);
}

void willi_training_fx_hide(willi_training_fx_t *fx)
{
    set_hidden_obj_array(fx->trail_green, WILLI_TRAIL_COUNT, true);
    set_hidden_obj_array(fx->trail_blue, WILLI_TRAIL_COUNT, true);
    set_hidden(fx->travel_dot_green, true);
    set_hidden(fx->travel_dot_blue, true);
}

void willi_training_fx_start(willi_training_fx_t *fx)
{
    if(!fx->timer)
        fx->timer = lv_timer_create(fx_timer_cb, 28, fx);
}

void willi_training_fx_stop(willi_training_fx_t *fx)
{
    if(fx->timer) {
        lv_timer_del(fx->timer);
        fx->timer = NULL;
    }
}