#include "willi_metric_card.h"

static void anim_y_cb(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, (lv_coord_t)v);
}

static void anim_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

void willi_metric_card_create(willi_metric_card_t *card, lv_obj_t *parent)
{
    if(!card) return;

    card->root = lv_obj_create(parent);
    lv_obj_remove_style_all(card->root);
    lv_obj_set_size(card->root, 140, 130);
    lv_obj_clear_flag(card->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(card->root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(card->root, 0, 0);
    lv_obj_set_style_pad_all(card->root, 0, 0);

    card->label_title = lv_label_create(card->root);
    lv_label_set_text(card->label_title, "TITLE");
    lv_obj_set_style_text_font(card->label_title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(card->label_title, lv_color_hex(0xEAEAEA), 0);
    lv_obj_align(card->label_title, LV_ALIGN_TOP_MID, 0, 0);

    card->label_value = lv_label_create(card->root);
    lv_label_set_text(card->label_value, "0.0");
    lv_obj_set_style_text_font(card->label_value, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(card->label_value, lv_color_hex(0x8DFF2F), 0);
    lv_obj_align(card->label_value, LV_ALIGN_TOP_MID, 0, 26);

    card->label_unit = lv_label_create(card->root);
    lv_label_set_text(card->label_unit, "unit");
    lv_obj_set_style_text_font(card->label_unit, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(card->label_unit, lv_color_hex(0xD8D8D8), 0);
    lv_obj_align(card->label_unit, LV_ALIGN_TOP_MID, 0, 86);

    card->final_x = 0;
    card->final_y = 0;
}

void willi_metric_card_set_title(willi_metric_card_t *card, const char *txt)
{
    if(card && card->label_title) lv_label_set_text(card->label_title, txt ? txt : "");
}

void willi_metric_card_set_value(willi_metric_card_t *card, const char *txt)
{
    if(card && card->label_value) lv_label_set_text(card->label_value, txt ? txt : "");
}

void willi_metric_card_set_unit(willi_metric_card_t *card, const char *txt)
{
    if(card && card->label_unit) lv_label_set_text(card->label_unit, txt ? txt : "");
}

void willi_metric_card_set_title_color(willi_metric_card_t *card, lv_color_t color)
{
    if(card && card->label_title) lv_obj_set_style_text_color(card->label_title, color, 0);
}

void willi_metric_card_set_value_color(willi_metric_card_t *card, lv_color_t color)
{
    if(card && card->label_value) lv_obj_set_style_text_color(card->label_value, color, 0);
}

void willi_metric_card_set_unit_color(willi_metric_card_t *card, lv_color_t color)
{
    if(card && card->label_unit) lv_obj_set_style_text_color(card->label_unit, color, 0);
}

void willi_metric_card_set_pos(willi_metric_card_t *card, lv_coord_t x, lv_coord_t y)
{
    if(!card || !card->root) return;
    lv_obj_set_pos(card->root, x, y);
    card->final_x = x;
    card->final_y = y;
}

void willi_metric_card_set_size(willi_metric_card_t *card, lv_coord_t w, lv_coord_t h)
{
    if(card && card->root) lv_obj_set_size(card->root, w, h);
}

void willi_metric_card_hide(willi_metric_card_t *card)
{
    if(card && card->root) lv_obj_add_flag(card->root, LV_OBJ_FLAG_HIDDEN);
}

void willi_metric_card_show(willi_metric_card_t *card)
{
    if(card && card->root) {
        lv_obj_clear_flag(card->root, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(card->root, LV_OPA_COVER, 0);
    }
}

void willi_metric_card_prepare_in_anim(willi_metric_card_t *card, lv_coord_t y_offset)
{
    if(!card || !card->root) return;

    lv_obj_clear_flag(card->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(card->root, card->final_x, card->final_y + y_offset);
    lv_obj_set_style_opa(card->root, LV_OPA_0, 0);
}

void willi_metric_card_animate_in(willi_metric_card_t *card, uint32_t delay_ms)
{
    if(!card || !card->root) return;

    lv_anim_t a;

    lv_anim_init(&a);
    lv_anim_set_var(&a, card->root);
    lv_anim_set_exec_cb(&a, anim_y_cb);
    lv_anim_set_values(&a, lv_obj_get_y(card->root), card->final_y);
    lv_anim_set_time(&a, 420);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, card->root);
    lv_anim_set_exec_cb(&a, anim_opa_cb);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_time(&a, 280);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
}

void willi_metric_card_prepare_out_anim(willi_metric_card_t *card, lv_coord_t y_offset)
{
    if(!card || !card->root) return;

    lv_obj_clear_flag(card->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(card->root, card->final_x, card->final_y);
    lv_obj_set_style_opa(card->root, LV_OPA_COVER, 0);

    (void)y_offset;
}

void willi_metric_card_animate_out(willi_metric_card_t *card, uint32_t delay_ms)
{
    if(!card || !card->root) return;

    lv_anim_t a;

    lv_anim_init(&a);
    lv_anim_set_var(&a, card->root);
    lv_anim_set_exec_cb(&a, anim_y_cb);
    lv_anim_set_values(&a, lv_obj_get_y(card->root), card->final_y - 16);
    lv_anim_set_time(&a, 220);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, card->root);
    lv_anim_set_exec_cb(&a, anim_opa_cb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_0);
    lv_anim_set_time(&a, 180);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
}