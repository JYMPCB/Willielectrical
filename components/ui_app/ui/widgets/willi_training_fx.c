#include "../../../willi_common/include/willi_opacity.h"
#include "willi_training_fx.h"
#include <math.h>
#include "lvgl/src/misc/lv_timer_private.h"

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
    lv_obj_set_style_shadow_spread(obj, 0, 0);
    lv_obj_set_style_shadow_color(obj, color, 0);
    lv_obj_set_style_shadow_opa(obj, opa, 0);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void build_u_wave(lv_point_t *pts,
                         uint16_t count,
                         lv_coord_t x_left,
                         lv_coord_t x_right,
                         lv_coord_t y_base,
                         lv_coord_t depth)
{
    if(!pts || count < 2) return;

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

    if(!pts || count < 2) return out;

    if(pos < 0.0f) pos = 0.0f;
    if(pos > (float)(count - 1)) pos = (float)(count - 1);

    {
        int idx = (int)pos;
        float frac = pos - (float)idx;
        int next = idx + 1;
        if(next >= count) next = count - 1;

        out.x = (lv_coord_t)((float)pts[idx].x + ((float)pts[next].x - (float)pts[idx].x) * frac);
        out.y = (lv_coord_t)((float)pts[idx].y + ((float)pts[next].y - (float)pts[idx].y) * frac);
    }

    return out;
}

static void place_dot_pair(lv_obj_t *halo, lv_obj_t *dot, lv_point_t p, lv_coord_t halo_r, lv_coord_t dot_r)
{
    if(halo) lv_obj_set_pos(halo, p.x - halo_r, p.y - halo_r);
    if(dot)  lv_obj_set_pos(dot, p.x - dot_r, p.y - dot_r);
}

static void create_trail_particles(lv_obj_t *parent, lv_obj_t **arr, uint16_t count, lv_color_t color)
{
    if(!parent || !arr) return;

    for(uint16_t i = 0; i < count; i++) {
        lv_coord_t size;
        lv_opa_t opa;

        if(i == 0) {
            size = 6;
            opa = OPA_44;
        } else if(i < 3) {
            size = 5;
            opa = OPA_32;
        } else if(i < 5) {
            size = 4;
            opa = OPA_24;
        } else {
            size = 3;
            opa = OPA_16;
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
    if(!arr || !pts || pts_count < 2) return;

    for(uint16_t i = 0; i < count; i++) {
        float pos = head_pos - ((float)(i + 1) * spacing);

        while(pos < 0.0f) {
            pos += (float)(pts_count - 1);
        }

        lv_point_t p = interp_point(pts, pts_count, pos);

        lv_coord_t r;
        if(i == 0) r = base_radius;
        else if(i < 3) r = base_radius - 1;
        else r = base_radius - 2;

        if(r < 1) r = 1;

        if(arr[i]) {
            lv_obj_set_pos(arr[i], p.x - r, p.y - r);
        }
    }
}

static void fx_timer_cb(lv_timer_t *t)
{
    willi_training_fx_t *fx = (willi_training_fx_t *)t->user_data;
    if(!fx) return;

    fx->travel_pos_green += 0.28f;
    fx->travel_pos_blue  += 0.18f;

    if(fx->travel_pos_green > (float)(WILLI_WAVE_PTS - 1)) fx->travel_pos_green = 0.0f;
    if(fx->travel_pos_blue  > (float)(WILLI_WAVE_PTS - 1)) fx->travel_pos_blue  = 0.0f;

    {
        lv_point_t pg = interp_point(fx->wave_green_pts, WILLI_WAVE_PTS, fx->travel_pos_green);
        lv_point_t pb = interp_point(fx->wave_blue_pts,  WILLI_WAVE_PTS, fx->travel_pos_blue);

        place_dot_pair(fx->travel_dot_green_halo, fx->travel_dot_green, pg, 8, 4);
        place_dot_pair(fx->travel_dot_blue_halo,  fx->travel_dot_blue,  pb, 6, 3);

        place_trail_particles(fx->trail_green, WILLI_TRAIL_COUNT,
                              fx->wave_green_pts, WILLI_WAVE_PTS,
                              fx->travel_pos_green, 1.3f, 3);

        place_trail_particles(fx->trail_blue, WILLI_TRAIL_COUNT,
                              fx->wave_blue_pts, WILLI_WAVE_PTS,
                              fx->travel_pos_blue, 1.1f, 2);
    }
}

void willi_training_fx_create(lv_obj_t *parent,
                              willi_training_fx_t *fx,
                              lv_coord_t x_left,
                              lv_coord_t x_right,
                              lv_coord_t green_base_y,
                              lv_coord_t green_depth,
                              lv_coord_t blue_base_y,
                              lv_coord_t blue_depth)
{
    if(!parent || !fx) return;

    for(uint16_t i = 0; i < WILLI_TRAIL_COUNT; i++) {
        fx->trail_green[i] = NULL;
        fx->trail_blue[i] = NULL;
    }

    fx->timer = NULL;
    fx->travel_pos_green = 8.0f;
    fx->travel_pos_blue  = 33.0f;

    fx->x_left = x_left;
    fx->x_right = x_right;
    fx->green_base_y = green_base_y;
    fx->green_depth = green_depth;
    fx->blue_base_y = blue_base_y;
    fx->blue_depth = blue_depth;

    build_u_wave(fx->wave_green_pts, WILLI_WAVE_PTS, x_left, x_right, green_base_y, green_depth);
    build_u_wave(fx->wave_blue_pts,  WILLI_WAVE_PTS, x_left + 52, x_right - 56, blue_base_y, blue_depth);

    create_trail_particles(parent, fx->trail_green, WILLI_TRAIL_COUNT, lv_color_hex(0x7CFF72));
    create_trail_particles(parent, fx->trail_blue,  WILLI_TRAIL_COUNT, lv_color_hex(0x37D8FF));

    fx->travel_dot_green_halo = lv_obj_create(parent);
    style_halo_dot(fx->travel_dot_green_halo, lv_color_hex(0x88FF78), 16, OPA_20);

    fx->travel_dot_green = lv_obj_create(parent);
    style_travel_dot(fx->travel_dot_green, lv_color_hex(0x8AFF72), 8, OPA_88);

    fx->travel_dot_blue_halo = lv_obj_create(parent);
    style_halo_dot(fx->travel_dot_blue_halo, lv_color_hex(0x37D8FF), 12, OPA_18);

    fx->travel_dot_blue = lv_obj_create(parent);
    style_travel_dot(fx->travel_dot_blue, lv_color_hex(0x37D8FF), 6, OPA_76);

    {
        lv_point_t pg = interp_point(fx->wave_green_pts, WILLI_WAVE_PTS, fx->travel_pos_green);
        lv_point_t pb = interp_point(fx->wave_blue_pts,  WILLI_WAVE_PTS, fx->travel_pos_blue);

        place_dot_pair(fx->travel_dot_green_halo, fx->travel_dot_green, pg, 8, 4);
        place_dot_pair(fx->travel_dot_blue_halo,  fx->travel_dot_blue,  pb, 6, 3);

        place_trail_particles(fx->trail_green, WILLI_TRAIL_COUNT,
                              fx->wave_green_pts, WILLI_WAVE_PTS,
                              fx->travel_pos_green, 1.3f, 3);

        place_trail_particles(fx->trail_blue, WILLI_TRAIL_COUNT,
                              fx->wave_blue_pts, WILLI_WAVE_PTS,
                              fx->travel_pos_blue, 1.1f, 2);
    }

    move_obj_array_fg(fx->trail_green, WILLI_TRAIL_COUNT);
    move_obj_array_fg(fx->trail_blue, WILLI_TRAIL_COUNT);
    lv_obj_move_foreground(fx->travel_dot_green_halo);
    lv_obj_move_foreground(fx->travel_dot_blue_halo);
    lv_obj_move_foreground(fx->travel_dot_green);
    lv_obj_move_foreground(fx->travel_dot_blue);
}

void willi_training_fx_show(willi_training_fx_t *fx)
{
    if(!fx) return;

    set_hidden_obj_array(fx->trail_green, WILLI_TRAIL_COUNT, false);
    set_hidden_obj_array(fx->trail_blue, WILLI_TRAIL_COUNT, false);

    set_hidden(fx->travel_dot_green_halo, false);
    set_hidden(fx->travel_dot_green, false);
    set_hidden(fx->travel_dot_blue_halo, false);
    set_hidden(fx->travel_dot_blue, false);
}

void willi_training_fx_hide(willi_training_fx_t *fx)
{
    if(!fx) return;

    set_hidden_obj_array(fx->trail_green, WILLI_TRAIL_COUNT, true);
    set_hidden_obj_array(fx->trail_blue, WILLI_TRAIL_COUNT, true);

    set_hidden(fx->travel_dot_green_halo, true);
    set_hidden(fx->travel_dot_green, true);
    set_hidden(fx->travel_dot_blue_halo, true);
    set_hidden(fx->travel_dot_blue, true);
}

void willi_training_fx_start(willi_training_fx_t *fx)
{
    if(!fx) return;

    if(!fx->timer) {
        fx->timer = lv_timer_create(fx_timer_cb, 28, fx);
    }
}

void willi_training_fx_stop(willi_training_fx_t *fx)
{
    if(!fx) return;

    if(fx->timer) {
        lv_timer_del(fx->timer);
        fx->timer = NULL;
    }
}