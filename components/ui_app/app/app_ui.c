#include "app_ui.h"
#include "../ui/screens/ui_home.h"
#include "lvgl.h"

void app_ui_init(void)
{
    lv_obj_t *scr = ui_home_create();
    lv_scr_load(scr);
}

/*app_ui_init de prueba comentar
void app_ui_init(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "UI OK");
    lv_obj_center(label);
    lv_scr_load(scr);
}*/