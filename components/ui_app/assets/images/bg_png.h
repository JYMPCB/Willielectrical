#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#if LVGL_VERSION_MAJOR >= 9
extern const lv_image_dsc_t ui_bg_png;
#else
extern const lv_img_dsc_t ui_bg_png;
#endif

#ifdef __cplusplus
}
#endif
