# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
	components/lcd/display_port
	components/lcd/MIPI
	components/lcd/touch
)

set(COMPONENTS
	main
	app
	willi_common
	display_port
	MIPI
	touch
	ui
	tasks
	audio
	wifi_mgr
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Will I electrical)

if(WIN32)
	add_custom_target(sync_ota_manifest
		COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/tools/sync_ota_manifest.ps1
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Sync OTA latest.json from FW_VERSION"
		VERBATIM
	)

	if(TARGET app)
		add_dependencies(app sync_ota_manifest)
	endif()
endif()
