#Standalone tools built from engine sources: imgtool, iqmtool, httpserver, ftemaster. Plus the radiant map editor.

set(_E "${FTE_ROOT}/engine")

if(FTE_TOOL_IMAGE)
	add_executable(imgtool
		${FTE_ROOT}/imgtool.c
		${_E}/client/image.c
	)
	target_compile_definitions(imgtool PRIVATE IMGTOOL)
	target_link_libraries(imgtool PRIVATE fte_compile_options fte_platform_client fte_deps_client)
	fte_setup_target(imgtool)
endif()

if(FTE_TOOL_IQM)
	add_executable(iqmtool
		${FTE_ROOT}/iqm/iqm.cpp
		${FTE_ROOT}/iqm/iqm.h
		${FTE_ROOT}/plugins/models/gltf.c
		${FTE_ROOT}/imgtool.c
		${_E}/common/json.c
		${_E}/client/image.c
	)
	target_compile_definitions(iqmtool PRIVATE IQMTOOL)
	target_link_libraries(iqmtool PRIVATE fte_compile_options fte_deps_client ${CMAKE_DL_LIBS})
	fte_setup_target(iqmtool)
endif()

if(FTE_TOOL_HTTPSV)
	add_executable(httpserver
		${_E}/common/fs_stdio.c
		${_E}/http/httpserver.c
		${_E}/http/iwebiface.c
		${_E}/http/ftpserver.c
	)
	target_compile_definitions(httpserver PRIVATE WEBSERVER WEBSVONLY)
	target_link_libraries(httpserver PRIVATE fte_compile_options)
	if(WIN32)
		target_link_libraries(httpserver PRIVATE ws2_32)
	endif()
	fte_setup_target(httpserver)
endif()

if(FTE_TOOL_MASTER)
	add_executable(ftemaster
		${FTE_SERVER_ARCH_FILES}
		${_E}/server/sv_master.c
		${_E}/common/net_ice.c		#stun responses
		${_E}/common/net_wins.c
		${_E}/common/cvar.c
		${_E}/common/cmd.c
		${_E}/common/sha1.c			#websockets
		${_E}/common/sha2.c			#fingerprints
		${_E}/http/httpclient.c		#pipe stuff
		${_E}/common/log.c
		${_E}/common/fs.c
		${_E}/common/fs_stdio.c
		${_E}/common/common.c
		${_E}/common/translate.c
		${_E}/common/zone.c
		${_E}/qclib/hash.c
	)
	target_compile_definitions(ftemaster PRIVATE MASTERONLY)
	target_link_libraries(ftemaster PRIVATE fte_compile_options fte_platform_server fte_deps_base)
	fte_setup_target(ftemaster)
endif()

unset(_E)

#radiant: the map editor. SDL2 window, OpenGL3 + Dear ImGui (docking) for the UI, Vulkan linked from the SDK.
if(FTE_TOOL_RADIANT)
	find_package(SDL2 CONFIG REQUIRED)
	find_package(imgui CONFIG REQUIRED)
	find_package(OpenGL REQUIRED)
	find_package(Vulkan REQUIRED)	#FindVulkan: uses $VULKAN_SDK on Windows, the system package elsewhere

	set(_R "${CMAKE_CURRENT_SOURCE_DIR}/radiant")
	set(RADIANT_SOURCES
		${_R}/main.cpp
		${_R}/config.h
		${_R}/mathlib.h

		#map data: grid now, brushes and entities later
		${_R}/map/grid.cpp
		${_R}/map/grid.h

		#gl: loader, shaders, framebuffers, cameras, textures
		${_R}/renderer/glFuncs.cpp
		${_R}/renderer/glFuncs.h
		${_R}/renderer/shader.cpp
		${_R}/renderer/shader.h
		${_R}/renderer/framebuffer.cpp
		${_R}/renderer/framebuffer.h
		${_R}/renderer/camera.cpp
		${_R}/renderer/camera.h
		${_R}/renderer/orthographicCamera.cpp
		${_R}/renderer/orthographicCamera.h
		${_R}/renderer/gridRenderer.cpp
		${_R}/renderer/gridRenderer.h
		${_R}/renderer/texture.cpp
		${_R}/renderer/texture.h
		${_R}/renderer/textures.cpp
		${_R}/renderer/textures.h

		#main frame: menu, toolbar, status bar, docking layout
		${_R}/ui/window.cpp
		${_R}/ui/window.h
		${_R}/ui/wndMain.cpp
		${_R}/ui/wndMain.h
		${_R}/ui/wndMain_menu.cpp
		${_R}/ui/wndMain_status.cpp
		${_R}/ui/wndMain_toolbar.cpp

		${_R}/ui/favourites/textureFavorites.cpp
		${_R}/ui/favourites/textureFavorites.h

		${_R}/ui/textures/textureView.cpp
		${_R}/ui/textures/textureView.h
		${_R}/ui/textures/wndTextures.cpp
		${_R}/ui/textures/wndTextures.h

		${_R}/ui/view/mapView2D.cpp
		${_R}/ui/view/mapView2D.h
		${_R}/ui/view/wndCamera.cpp
		${_R}/ui/view/wndCamera.h
	)
	add_executable(radiant WIN32 ${RADIANT_SOURCES})
	target_include_directories(radiant PRIVATE ${_R})	#includes are relative to tools/radiant: "ui/window.h"
	source_group(TREE ${_R} FILES ${RADIANT_SOURCES})	#same folders in the IDE

	#toolbar icons (quaked3's toolbar2.bmp), loaded from <exe dir>/radiant/
	add_custom_command(TARGET radiant POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_R}/res/toolbar.bmp $<TARGET_FILE_DIR:radiant>/radiant/toolbar.bmp)
	unset(_R)
	target_link_libraries(radiant PRIVATE
		$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
		$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
		imgui::imgui
		OpenGL::GL
		Vulkan::Vulkan
	)

	set_target_properties(radiant PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF)
	if(MSVC)
		target_compile_options(radiant PRIVATE /W4 /permissive- /utf-8)
	else()
		target_compile_options(radiant PRIVATE -Wall -Wextra)
	endif()

	fte_setup_target(radiant)
	set_target_properties(radiant PROPERTIES FOLDER "Editor")
endif()
