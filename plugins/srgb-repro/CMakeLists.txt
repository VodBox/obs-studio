project(srgb-repro)

set(srgb-repro_SOURCES
	srgb-repro.c)
	
add_library(srgb-repro MODULE
	${srgb-repro_SOURCES})
	
target_link_libraries(srgb-repro
	libobs)

set_target_properties(srgb-repro PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(srgb-repro data)