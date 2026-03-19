#include "willi_training_metrics.h"

#define C_TITLE   0xEAEAEA
#define C_WHITE   0xF2F2F2
#define C_GREEN   0x8DFF2F
#define C_BLUE    0x61B8FF
#define C_ORANGE  0xFFB12A

static void setup_card(willi_metric_card_t *card,
                       const char *title,
                       const char *value,
                       const char *unit,
                       uint32_t value_color)
{
    willi_metric_card_set_title(card, title);
    willi_metric_card_set_value(card, value);
    willi_metric_card_set_unit(card, unit);
    willi_metric_card_set_title_color(card, lv_color_hex(C_TITLE));
    willi_metric_card_set_unit_color(card, lv_color_hex(C_TITLE));
    willi_metric_card_set_value_color(card, lv_color_hex(value_color));
}

void willi_training_metrics_create(lv_obj_t *parent, willi_training_metrics_t *m)
{
    if(!m) return;

    willi_metric_card_create(&m->speed, parent);
    willi_metric_card_set_pos(&m->speed, 95, 110);
    willi_metric_card_set_size(&m->speed, 165, 130);
    setup_card(&m->speed, "VELOCIDAD", "8.5", "km/h", C_GREEN);

    willi_metric_card_create(&m->pace, parent);
    willi_metric_card_set_pos(&m->pace, 265, 110);
    willi_metric_card_set_size(&m->pace, 150, 130);
    setup_card(&m->pace, "RITMO", "07:03", "min/km", C_BLUE);

    willi_metric_card_create(&m->distance, parent);
    willi_metric_card_set_pos(&m->distance, 440, 110);
    willi_metric_card_set_size(&m->distance, 150, 130);
    setup_card(&m->distance, "DISTANCIA", "2.54", "km", C_WHITE);

    willi_metric_card_create(&m->time, parent);
    willi_metric_card_set_pos(&m->time, 610, 110);
    willi_metric_card_set_size(&m->time, 185, 130);
    setup_card(&m->time, "TIEMPO", "00:18:33", "", C_WHITE);

    lv_obj_set_style_text_font(m->time.label_value, &lv_font_montserrat_40, 0);
    lv_obj_align(m->time.label_value, LV_ALIGN_TOP_MID, 0, 34);

    willi_metric_card_create(&m->calories, parent);
    willi_metric_card_set_pos(&m->calories, 795, 110);
    willi_metric_card_set_size(&m->calories, 150, 130);
    setup_card(&m->calories, "CALORIAS", "174", "kcal", C_ORANGE);
}

void willi_training_metrics_hide_all(willi_training_metrics_t *m)
{
    if(!m) return;
    willi_metric_card_hide(&m->speed);
    willi_metric_card_hide(&m->pace);
    willi_metric_card_hide(&m->distance);
    willi_metric_card_hide(&m->time);
    willi_metric_card_hide(&m->calories);
}

void willi_training_metrics_show_animated(willi_training_metrics_t *m)
{
    if(!m) return;

    willi_metric_card_prepare_in_anim(&m->speed, 24);
    willi_metric_card_prepare_in_anim(&m->pace, 24);
    willi_metric_card_prepare_in_anim(&m->distance, 24);
    willi_metric_card_prepare_in_anim(&m->time, 24);
    willi_metric_card_prepare_in_anim(&m->calories, 24);

    willi_metric_card_animate_in(&m->speed, 0);
    willi_metric_card_animate_in(&m->pace, 80);
    willi_metric_card_animate_in(&m->distance, 160);
    willi_metric_card_animate_in(&m->time, 240);
    willi_metric_card_animate_in(&m->calories, 320);
}

void willi_training_metrics_hide_animated(willi_training_metrics_t *m)
{
    if(!m) return;

    willi_metric_card_prepare_out_anim(&m->calories, 0);
    willi_metric_card_prepare_out_anim(&m->time, 0);
    willi_metric_card_prepare_out_anim(&m->distance, 0);
    willi_metric_card_prepare_out_anim(&m->pace, 0);
    willi_metric_card_prepare_out_anim(&m->speed, 0);

    willi_metric_card_animate_out(&m->calories, 0);
    willi_metric_card_animate_out(&m->time, 40);
    willi_metric_card_animate_out(&m->distance, 80);
    willi_metric_card_animate_out(&m->pace, 120);
    willi_metric_card_animate_out(&m->speed, 160);
}