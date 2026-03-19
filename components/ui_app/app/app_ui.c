#include "app_ui.h"
#include "../ui/screens/ui_home.h"
#include "lvgl.h"

void app_ui_init(void)
{
    lv_obj_t *scr = ui_home_create();
    lv_scr_load(scr);
}