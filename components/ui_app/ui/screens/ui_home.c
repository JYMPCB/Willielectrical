#include "ui_home.h"
#include "../../assets/images/img_bg.h"
#include "../widgets/willi_start_btn.h"
#include "../widgets/willi_program_btn.h"
#include "../widgets/willi_free_btn.h"
#include "../widgets/willi_interval_btn.h"
#include "../widgets/willi_training_metrics.h"
#include "../widgets/willi_stepper_btn.h"
#include "../widgets/willi_stop_btn.h"
#include "../widgets/willi_back_btn.h"

typedef enum {
    HOME_MODE_PROGRAM = 0,
    HOME_MODE_FREE,
    HOME_MODE_INTERVAL
} home_mode_t;

typedef enum {
    HOME_VISUAL_IDLE = 0,
    HOME_VISUAL_RUNNING
} home_visual_state_t;

static home_mode_t s_home_mode = HOME_MODE_FREE;
static home_visual_state_t s_visual_state = HOME_VISUAL_IDLE;

static lv_obj_t *s_btn_program = NULL;
static lv_obj_t *s_btn_free = NULL;
static lv_obj_t *s_btn_interval = NULL;
static lv_obj_t *s_btn_back = NULL;

static willi_start_btn_t   *s_btn_start = NULL;
static willi_stepper_btn_t *s_btn_speed = NULL;
static willi_stepper_btn_t *s_btn_pace  = NULL;
static willi_stop_btn_t    *s_btn_stop  = NULL;

static willi_training_metrics_t s_metrics;

static willi_wifi_status_t s_wifi;
static lv_obj_t *s_clock_label = NULL;

/* posiciones reales */
static lv_coord_t s_pos_program_y = 0;
static lv_coord_t s_pos_free_y = 0;
static lv_coord_t s_pos_interval_y = 0;
static lv_coord_t s_pos_start_y = 0;

static lv_coord_t s_pos_run_left_y = 0;
static lv_coord_t s_pos_run_stop_y = 0;
static lv_coord_t s_pos_run_right_y = 0;
static lv_coord_t s_pos_back_y = 0;

/* ajuste fino */
#define RUN_BTN_Y_TUNE   (-26)

static lv_obj_t *home_back_root(void)
{
    return s_btn_back;
}

static lv_obj_t *home_start_root(void)
{
    return willi_start_btn_get_root(s_btn_start);
}

static lv_obj_t *home_speed_root(void)
{
    return willi_stepper_btn_get_root(s_btn_speed);
}

static lv_obj_t *home_pace_root(void)
{
    return willi_stepper_btn_get_root(s_btn_pace);
}

static lv_obj_t *home_stop_root(void)
{
    return willi_stop_btn_get_root(s_btn_stop);
}

static void home_enter_running_ui(void);
static void home_exit_running_ui(void);

static void home_apply_mode(home_mode_t mode)
{
    s_home_mode = mode;

    if(s_btn_program)  lv_obj_clear_state(s_btn_program,  LV_STATE_CHECKED);
    if(s_btn_free)     lv_obj_clear_state(s_btn_free,     LV_STATE_CHECKED);
    if(s_btn_interval) lv_obj_clear_state(s_btn_interval, LV_STATE_CHECKED);

    switch(mode) {
        case HOME_MODE_PROGRAM:
            if(s_btn_program) lv_obj_add_state(s_btn_program, LV_STATE_CHECKED);
            break;

        case HOME_MODE_FREE:
            if(s_btn_free) lv_obj_add_state(s_btn_free, LV_STATE_CHECKED);
            break;

        case HOME_MODE_INTERVAL:
            if(s_btn_interval) lv_obj_add_state(s_btn_interval, LV_STATE_CHECKED);
            break;
    }

    if(s_btn_start) {
        willi_start_btn_set_mode(s_btn_start, (willi_start_mode_t)mode);
    }
}

static void home_back_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    home_exit_running_ui();
}

static void home_program_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(s_visual_state != HOME_VISUAL_IDLE) return;
    home_apply_mode(HOME_MODE_PROGRAM);
}

static void home_free_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(s_visual_state != HOME_VISUAL_IDLE) return;
    home_apply_mode(HOME_MODE_FREE);
}

static void home_interval_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(s_visual_state != HOME_VISUAL_IDLE) return;
    home_apply_mode(HOME_MODE_INTERVAL);
}

static void home_start_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    home_enter_running_ui();
}

static void home_stop_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    home_exit_running_ui();
}

/* ---------------- anim helpers ---------------- */

static void anim_obj_y_cb(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, (lv_coord_t)v);
}

static void anim_obj_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

static void hide_obj_ready_cb(lv_anim_t *a)
{
    lv_obj_t *obj = (lv_obj_t *)a->var;
    if(obj) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void stop_obj_anims(lv_obj_t *obj)
{
    if(!obj) return;
    lv_anim_del(obj, anim_obj_y_cb);
    lv_anim_del(obj, anim_obj_opa_cb);
}

static void obj_anim_in_from_bottom(lv_obj_t *obj, lv_coord_t final_y, lv_coord_t offset, uint32_t delay, uint32_t time)
{
    if(!obj) return;

    stop_obj_anims(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_y(obj, final_y + offset);
    lv_obj_set_style_opa(obj, LV_OPA_0, 0);

    lv_anim_t a;

    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_obj_y_cb);
    lv_anim_set_values(&a, final_y + offset, final_y);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_obj_opa_cb);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_time(&a, (time > 80) ? (time - 80) : time);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
}

static void obj_anim_out_to_top(lv_obj_t *obj, lv_coord_t final_y, lv_coord_t offset, uint32_t delay, uint32_t time)
{
    if(!obj) return;

    stop_obj_anims(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_y(obj, final_y);
    lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);

    lv_anim_t a;

    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_obj_y_cb);
    lv_anim_set_values(&a, final_y, final_y - offset);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_obj_opa_cb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_0);
    lv_anim_set_time(&a, (time > 60) ? (time - 60) : time);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_ready_cb(&a, hide_obj_ready_cb);
    lv_anim_start(&a);
}

static void home_place_idle_layout(void)
{
    lv_obj_align(s_btn_program,      LV_ALIGN_BOTTOM_LEFT,   32, s_pos_program_y);
    lv_obj_align(s_btn_free,         LV_ALIGN_BOTTOM_MID,     0, s_pos_free_y);
    lv_obj_align(s_btn_interval,     LV_ALIGN_BOTTOM_RIGHT, -32, s_pos_interval_y);
    lv_obj_align(home_start_root(),  LV_ALIGN_CENTER,         0, s_pos_start_y);
}

static void home_place_running_layout(void)
{
    lv_obj_align(home_speed_root(), LV_ALIGN_BOTTOM_LEFT,   70, s_pos_run_left_y  + RUN_BTN_Y_TUNE);
    lv_obj_align(home_stop_root(),  LV_ALIGN_BOTTOM_MID,     0, s_pos_run_stop_y  + RUN_BTN_Y_TUNE);
    lv_obj_align(home_pace_root(),  LV_ALIGN_BOTTOM_RIGHT, -70, s_pos_run_right_y + RUN_BTN_Y_TUNE);
    lv_obj_align(home_back_root(),  LV_ALIGN_TOP_MID,      0, s_pos_back_y);
}

static void home_restore_idle_layout(void)
{
    stop_obj_anims(s_btn_program);
    stop_obj_anims(s_btn_free);
    stop_obj_anims(s_btn_interval);
    stop_obj_anims(home_start_root());
    stop_obj_anims(home_speed_root());
    stop_obj_anims(home_stop_root());
    stop_obj_anims(home_pace_root());
    stop_obj_anims(home_back_root());

    home_place_idle_layout();
    home_place_running_layout();

    lv_obj_set_style_opa(s_btn_program, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(s_btn_free, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(s_btn_interval, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(home_start_root(), LV_OPA_COVER, 0);
    lv_obj_set_style_opa(home_speed_root(), LV_OPA_COVER, 0);
    lv_obj_set_style_opa(home_stop_root(), LV_OPA_COVER, 0);
    lv_obj_set_style_opa(home_pace_root(), LV_OPA_COVER, 0);
    lv_obj_set_style_opa(home_back_root(), LV_OPA_COVER, 0);

    lv_obj_clear_flag(s_btn_program, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_btn_free, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_btn_interval, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(home_start_root(), LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(home_speed_root(), LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(home_stop_root(), LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(home_pace_root(), LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(home_back_root(), LV_OBJ_FLAG_HIDDEN);
}

/* ---------------- transitions ---------------- */

static void home_enter_running_ui(void)
{
    if(s_visual_state == HOME_VISUAL_RUNNING) return;
    s_visual_state = HOME_VISUAL_RUNNING;

    obj_anim_out_to_top(s_btn_program,      s_pos_program_y,   20,  0, 220);
    obj_anim_out_to_top(s_btn_free,         s_pos_free_y,      20, 40, 220);
    obj_anim_out_to_top(s_btn_interval,     s_pos_interval_y,  20, 80, 220);
    obj_anim_out_to_top(home_start_root(),  s_pos_start_y,     24, 60, 240);

    willi_training_metrics_show_animated(&s_metrics);

    obj_anim_in_from_bottom(home_speed_root(), s_pos_run_left_y  + RUN_BTN_Y_TUNE, 30, 260, 360);
    obj_anim_in_from_bottom(home_stop_root(),  s_pos_run_stop_y  + RUN_BTN_Y_TUNE, 30, 340, 360);
    obj_anim_in_from_bottom(home_pace_root(),  s_pos_run_right_y + RUN_BTN_Y_TUNE, 30, 420, 360);
    obj_anim_in_from_bottom(home_back_root(),  s_pos_back_y, 18, 180, 260);
}

static void home_exit_running_ui(void)
{
    if(s_visual_state == HOME_VISUAL_IDLE) return;
    s_visual_state = HOME_VISUAL_IDLE;

    willi_training_metrics_hide_animated(&s_metrics);

    obj_anim_out_to_top(home_pace_root(),  s_pos_run_right_y + RUN_BTN_Y_TUNE, 18,  0, 200);
    obj_anim_out_to_top(home_stop_root(),  s_pos_run_stop_y  + RUN_BTN_Y_TUNE, 18, 40, 200);
    obj_anim_out_to_top(home_speed_root(), s_pos_run_left_y  + RUN_BTN_Y_TUNE, 18, 80, 200);
    obj_anim_out_to_top(home_back_root(),  s_pos_back_y, 18, 20, 180);

    obj_anim_in_from_bottom(s_btn_program,     s_pos_program_y,   24, 220, 320);
    obj_anim_in_from_bottom(s_btn_free,        s_pos_free_y,      24, 280, 320);
    obj_anim_in_from_bottom(s_btn_interval,    s_pos_interval_y,  24, 340, 320);
    obj_anim_in_from_bottom(home_start_root(), s_pos_start_y,     26, 300, 340);
}

/* ---------------- create screen ---------------- */

lv_obj_t *ui_home_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0D12), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x1B2230), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_HOR, 0);

    lv_obj_t *bg = lv_img_create(scr);
    lv_img_set_src(bg, &img_bg);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_CLICKABLE);

    lv_coord_t sw = lv_disp_get_hor_res(NULL);
    lv_coord_t sh = lv_disp_get_ver_res(NULL);
    uint32_t zoom_x = ((uint32_t)sw * 256U) / img_bg.header.w;
    uint32_t zoom_y = ((uint32_t)sh * 256U) / img_bg.header.h;
    uint32_t zoom = (zoom_x > zoom_y) ? zoom_x : zoom_y;
    if(zoom < 1U) zoom = 1U;
    if(zoom > 4096U) zoom = 4096U;
    lv_img_set_zoom(bg, (uint16_t)zoom);
    lv_obj_center(bg);
    lv_obj_move_background(bg);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "WILLI PRO");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 18);

    willi_wifi_status_create(scr, &s_wifi, 0, 0);
    willi_wifi_status_start(&s_wifi);

    s_clock_label = lv_label_create(scr);
    lv_label_set_text(s_clock_label, "10:45 AM");
    lv_obj_set_style_text_font(s_clock_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(s_clock_label, lv_color_hex(0xF2F2F2), 0);

    lv_obj_align(s_wifi.label, LV_ALIGN_TOP_RIGHT, -22, 20);
    lv_obj_align(s_clock_label, LV_ALIGN_TOP_RIGHT, -78, 20);

    s_btn_start    = willi_start_btn_create(scr);
    s_btn_program  = willi_program_btn_create(scr);
    s_btn_free     = willi_free_btn_create(scr);
    s_btn_interval = willi_interval_btn_create(scr);

    s_btn_speed = willi_stepper_btn_create(scr);
    willi_stepper_btn_set_title(s_btn_speed, "VELOCIDAD");
    willi_stepper_btn_set_accent(s_btn_speed, lv_color_hex(0x8DFF2D));
    willi_stepper_btn_set_role(s_btn_speed, WILLI_STEPPER_ROLE_SPEED);
    lv_obj_align(home_speed_root(), LV_ALIGN_BOTTOM_LEFT, 70, -40);
    lv_obj_add_flag(home_speed_root(), LV_OBJ_FLAG_HIDDEN);

    s_btn_pace = willi_stepper_btn_create(scr);
    willi_stepper_btn_set_title(s_btn_pace, "RITMO");
    willi_stepper_btn_set_accent(s_btn_pace, lv_color_hex(0x5AA7FF));
    willi_stepper_btn_set_role(s_btn_pace, WILLI_STEPPER_ROLE_PACE);
    lv_obj_align(home_pace_root(), LV_ALIGN_BOTTOM_RIGHT, -70, -40);
    lv_obj_add_flag(home_pace_root(), LV_OBJ_FLAG_HIDDEN);

    s_btn_stop = willi_stop_btn_create(scr);
    lv_obj_align(home_stop_root(), LV_ALIGN_BOTTOM_MID, 0, -42);
    lv_obj_add_flag(home_stop_root(), LV_OBJ_FLAG_HIDDEN);

    s_btn_back = willi_back_btn_create(scr);
    lv_obj_align(s_btn_back, LV_ALIGN_TOP_MID, 0, 42);
    lv_obj_add_flag(s_btn_back, LV_OBJ_FLAG_HIDDEN);

    willi_training_metrics_create(scr, &s_metrics);
    willi_training_metrics_hide_all(&s_metrics);

    willi_start_btn_start_breathing(s_btn_start);
    willi_start_btn_start_particles_anim(s_btn_start);
    willi_start_btn_start_spray_anim(s_btn_start);

    lv_obj_align(s_btn_program,      LV_ALIGN_BOTTOM_LEFT,   32, 0);
    lv_obj_align(s_btn_free,         LV_ALIGN_BOTTOM_MID,     0, 0);
    lv_obj_align(s_btn_interval,     LV_ALIGN_BOTTOM_RIGHT, -32, 0);
    lv_obj_align(home_start_root(),  LV_ALIGN_CENTER,         0, 0);

    /* guardar offsets lógicos, no posiciones resueltas */
    s_pos_program_y   = -50;
    s_pos_free_y      = -50;
    s_pos_interval_y  = -50;
    s_pos_start_y     = -36;

    s_pos_run_left_y  = -42;
    s_pos_run_stop_y  = -40;
    s_pos_run_right_y = -42;
    s_pos_back_y      = 2;   

    lv_obj_add_event_cb(s_btn_program,     home_program_btn_event_cb,  LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_free,        home_free_btn_event_cb,     LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_interval,    home_interval_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(home_start_root(), home_start_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(home_stop_root(),  home_stop_event_cb,  LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_back,        home_back_event_cb,  LV_EVENT_CLICKED, NULL);

    home_apply_mode(HOME_MODE_FREE);
    home_restore_idle_layout();

    return scr;
}