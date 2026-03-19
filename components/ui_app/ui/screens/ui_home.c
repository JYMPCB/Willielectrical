#include "ui_home.h"
#include "../../assets/images/bg_png.h"
#include "../widgets/willi_start_btn.h"

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

    willi_start_btn_t *start_btn = willi_start_btn_create(scr);
    willi_start_btn_start_breathing(start_btn);
    willi_start_btn_start_particles_anim(start_btn);
    willi_start_btn_start_spray_anim(start_btn);

    return scr;
}