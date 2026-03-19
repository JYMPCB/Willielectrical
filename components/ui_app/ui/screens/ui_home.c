#include "ui_home.h"
#include "../../assets/images/bg_png.h"
#include "../widgets/willi_start_btn.h"
#include "../widgets/willi_program_btn.h"
#include "../widgets/willi_free_btn.h"
#include "../widgets/willi_interval_btn.h"

typedef enum {
    HOME_MODE_PROGRAM = 0,
    HOME_MODE_FREE,
    HOME_MODE_INTERVAL
} home_mode_t;

static home_mode_t s_home_mode = HOME_MODE_FREE;

static lv_obj_t *s_btn_program = NULL;
static lv_obj_t *s_btn_free = NULL;
static lv_obj_t *s_btn_interval = NULL;
static willi_start_btn_t *s_btn_start = NULL;

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

static void home_program_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    home_apply_mode(HOME_MODE_PROGRAM);
}

static void home_free_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    home_apply_mode(HOME_MODE_FREE);
}

static void home_interval_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    home_apply_mode(HOME_MODE_INTERVAL);
}

lv_obj_t *ui_home_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0D12), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x1B2230), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_HOR, 0);

    lv_obj_t *bg = lv_img_create(scr);
    lv_img_set_src(bg, &ui_bg_png);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_CLICKABLE);

    lv_coord_t sw = lv_disp_get_hor_res(NULL);
    lv_coord_t sh = lv_disp_get_ver_res(NULL);
    uint32_t zoom_x = ((uint32_t)sw * 256U) / ui_bg_png.header.w;
    uint32_t zoom_y = ((uint32_t)sh * 256U) / ui_bg_png.header.h;
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

    lv_obj_t *time = lv_label_create(scr);
    lv_label_set_text(time, "10:45 AM");
    lv_obj_set_style_text_color(time, lv_color_white(), 0);
    lv_obj_set_style_text_font(time, &lv_font_montserrat_22, 0);
    lv_obj_align(time, LV_ALIGN_TOP_RIGHT, -24, 18);

    /* crear una sola vez */
    s_btn_start    = willi_start_btn_create(scr);
    s_btn_program  = willi_program_btn_create(scr);
    s_btn_free     = willi_free_btn_create(scr);
    s_btn_interval = willi_interval_btn_create(scr);

    /* arrancar animaciones del start */
    willi_start_btn_start_breathing(s_btn_start);
    willi_start_btn_start_particles_anim(s_btn_start);
    willi_start_btn_start_spray_anim(s_btn_start);

    /* alinear botones inferiores */
    lv_obj_align(s_btn_program,  LV_ALIGN_BOTTOM_LEFT,   32, -24);
    lv_obj_align(s_btn_free,     LV_ALIGN_BOTTOM_MID,     0, -24);
    lv_obj_align(s_btn_interval, LV_ALIGN_BOTTOM_RIGHT, -32, -24);

    /* eventos */
    lv_obj_add_event_cb(s_btn_program,  home_program_btn_event_cb,  LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_free,     home_free_btn_event_cb,     LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_btn_interval, home_interval_btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* modo por default */
    home_apply_mode(HOME_MODE_FREE);

    return scr;
}