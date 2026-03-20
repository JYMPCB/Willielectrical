set(ASSET_GEN_SCRIPT ${PROJECT_DIR}/tools/gen_lvgl_assets.py)

add_custom_target(generate_ui_assets ALL
    COMMAND ${PYTHON} ${ASSET_GEN_SCRIPT}
    WORKING_DIRECTORY ${PROJECT_DIR}
    COMMENT "Generating LVGL assets"
)

include(${CMAKE_CURRENT_LIST_DIR}/assets/images/assets.cmake OPTIONAL)

idf_component_register(
  SRCS
    "app/app_ui.c"
    ${UI_ASSET_SRCS}
    "ui/screens/ui_home.c"
    "ui/widgets/willi_start_btn.c"
  INCLUDE_DIRS
    "app"
    "assets/images"
    "ui/screens"
    "ui/widgets"
  REQUIRES
    lvgl
    willi_common
)

add_dependencies(${COMPONENT_LIB} generate_ui_assets)