

include(${CMAKE_CURRENT_LIST_DIR}/assets/images/assets.cmake OPTIONAL)

idf_component_register(
  SRCS
    "app/app_ui.c"
    ${UI_ASSET_SRCS}
    "ui/screens/ui_home.c"
    "ui/widgets/willi_start_btn.c"
    "ui_glue/src/ui_refresh.c"
  INCLUDE_DIRS
    "app"
    "assets/images"
    "assets/fonts"
    "ui/screens"
    "ui/widgets"
    "ui_glue/include"
    "../../managed_components/lvgl__lvgl"
    "${CMAKE_SOURCE_DIR}/components/app_core/include"
    "${CMAKE_SOURCE_DIR}/managed_components/lvgl__lvgl"

  REQUIRES
    lvgl
    willi_common
)

