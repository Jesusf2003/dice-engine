#Per-platform system layer: video/input/sound/threads/filesystem backends.
#
#Produces:
#  FTE_PLATFORM              - "win32", "sdl2" or "unix" (for the status line).
#  FTE_CLIENT_ARCH_FILES     - platform sources for client (and client+server) builds.
#  FTE_SERVER_ARCH_FILES     - platform sources for dedicated-server builds.
#  fte_platform_client       - INTERFACE target with the client's system libs/defines.
#  fte_platform_server       - INTERFACE target with the server's system libs/defines.

set(_E "${FTE_ROOT}/engine")
set(FTE_CLIENT_ARCH_FILES "")
set(FTE_SERVER_ARCH_FILES "")

add_library(fte_platform_client INTERFACE)
add_library(fte_platform_server INTERFACE)

if(FTE_USE_SDL)
	#--- SDL2 (any OS) ------------------------------------------------------
	set(FTE_PLATFORM "sdl2")
	find_package(SDL2 CONFIG REQUIRED)

	set(FTE_CLIENT_ARCH_FILES
		${_E}/client/sys_sdl.c
		${_E}/client/snd_al.c
		${_E}/client/snd_sdl.c
		${_E}/client/in_sdl.c
		${_E}/client/cd_sdl.c
		${_E}/gl/gl_vidsdl.c
	)
	target_compile_definitions(fte_platform_client INTERFACE FTE_SDL MULTITHREAD)
	target_link_libraries(fte_platform_client INTERFACE
		$<IF:$<TARGET_EXISTS:SDL2::SDL2-static>,SDL2::SDL2-static,SDL2::SDL2>
		$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
		${CMAKE_DL_LIBS})
	target_compile_definitions(fte_platform_server INTERFACE MULTITHREAD)

	if(WIN32)
		list(APPEND FTE_CLIENT_ARCH_FILES
			${_E}/client/winquake.rc
			${_E}/common/net_ssl_winsspi.c
			${_E}/common/fs_win32.c
		)
		set(FTE_SERVER_ARCH_FILES
			${_E}/client/winquake.rc
			${_E}/common/net_ssl_winsspi.c
			${_E}/common/fs_win32.c
			${_E}/server/sv_sys_win.c
		)
		target_compile_definitions(fte_platform_client INTERFACE NO_DIRECTX)
		target_link_libraries(fte_platform_client INTERFACE wsock32 ws2_32 gdi32 ole32 winmm)
		target_link_libraries(fte_platform_server INTERFACE wsock32 ws2_32 winmm)
	else()
		find_package(Threads REQUIRED)
		list(APPEND FTE_CLIENT_ARCH_FILES ${_E}/common/net_ssl_gnutls.c)
		set(FTE_SERVER_ARCH_FILES
			${_E}/common/net_ssl_gnutls.c
			${_E}/common/sys_linux_threads.c
			${_E}/server/sv_sys_unix.c
		)
		target_link_libraries(fte_platform_client INTERFACE m)
		target_link_libraries(fte_platform_server INTERFACE m ${CMAKE_DL_LIBS} Threads::Threads)
	endif()

elseif(WIN32)
	#--- Native Win32: GL + Vulkan + D3D8/9/11, DirectSound/WASAPI/XAudio/OpenAL --
	set(FTE_PLATFORM "win32")

	set(FTE_CLIENT_ARCH_FILES
		${_E}/client/winquake.rc
		${_E}/common/sys_win_threads.c
		${_E}/common/net_ssl_winsspi.c
		${_E}/common/net_ssl_gnutls.c
		${_E}/common/fs_win32.c
		${_E}/client/cd_win.c
		${_E}/client/in_win.c
		${_E}/client/snd_al.c
		${_E}/client/snd_directx.c
		${_E}/client/snd_wasapi.c
		${_E}/client/snd_win.c
		${_E}/client/snd_xaudio.c
		${_E}/client/sys_win.c

		${_E}/gl/gl_vidnt.c

		${_E}/d3d/d3d_backend.c
		${_E}/d3d/d3d_image.c
		${_E}/d3d/d3d_shader.c
		${_E}/d3d/d3d11_backend.c
		${_E}/d3d/d3d11_image.c
		${_E}/d3d/d3d11_shader.c
		${_E}/d3d/d3d8_backend.c
		${_E}/d3d/d3d8_image.c
		${_E}/d3d/vid_d3d.c
		${_E}/d3d/vid_d3d11.c
		${_E}/d3d/vid_d3d8.c
	)
	set(FTE_SERVER_ARCH_FILES
		${_E}/client/winquake.rc
		${_E}/common/sys_win_threads.c
		${_E}/common/net_ssl_winsspi.c
		${_E}/common/net_ssl_gnutls.c
		${_E}/common/fs_win32.c
		${_E}/server/sv_sys_win.c
	)
	#TLS/DTLS goes through SChannel; gnutls is only used on unix (net_ssl_gnutls.c compiles to nothing).
	target_compile_definitions(fte_platform_client INTERFACE D3D9QUAKE D3D11QUAKE NO_GNUTLS)
	target_compile_definitions(fte_platform_server INTERFACE NO_GNUTLS)
	target_link_libraries(fte_platform_client INTERFACE ole32 gdi32 wsock32 ws2_32 winmm dxguid)
	target_link_libraries(fte_platform_server INTERFACE wsock32 ws2_32 winmm)

	option(FTE_DEP_WINSSPI "Use SChannel (winsspi) for TLS/DTLS." ON)
	if(NOT FTE_DEP_WINSSPI)
		target_compile_definitions(fte_platform_client INTERFACE NO_WINSSPI)
		target_compile_definitions(fte_platform_server INTERFACE NO_WINSSPI)
	endif()

else()
	#--- Native Linux/BSD: GLX/EGL/Wayland, ALSA/Pulse/OSS ------------------
	set(FTE_PLATFORM "unix")
	find_package(Threads REQUIRED)

	set(FTE_CLIENT_ARCH_FILES
		${_E}/client/sys_linux.c
		${_E}/common/sys_linux_threads.c
		${_E}/common/net_ssl_gnutls.c
		${_E}/client/snd_al.c
		${_E}/client/snd_alsa.c
		${_E}/client/snd_linux.c
		${_E}/client/snd_pulse.c
		${_E}/client/snd_sdl.c
		${_E}/client/cd_linux.c
		${_E}/gl/gl_vidlinuxglx.c
		${_E}/gl/gl_videgl.c
	)
	set(FTE_SERVER_ARCH_FILES
		${_E}/server/sv_sys_unix.c
		${_E}/common/sys_linux_threads.c
		${_E}/common/net_ssl_gnutls.c
	)
	target_compile_definitions(fte_platform_client INTERFACE DYNAMIC_SDL MULTITHREAD)
	target_compile_definitions(fte_platform_server INTERFACE MULTITHREAD)
	target_link_libraries(fte_platform_client INTERFACE m ${CMAKE_DL_LIBS} Threads::Threads)
	target_link_libraries(fte_platform_server INTERFACE m ${CMAKE_DL_LIBS} Threads::Threads)

	find_package(ALSA)
	if(ALSA_FOUND)
		target_compile_definitions(fte_platform_client INTERFACE AUDIO_ALSA AUDIO_PULSE)
	endif()

	find_package(X11)
	if(X11_FOUND)
		target_include_directories(fte_platform_client INTERFACE ${X11_INCLUDE_DIR})
		if(NOT X11_Xcursor_FOUND)
			target_compile_definitions(fte_platform_client INTERFACE NO_X11_CURSOR)
		endif()
		if(NOT X11_Xrandr_FOUND)
			target_compile_definitions(fte_platform_client INTERFACE NO_X11_RANDR)
		endif()
		if(NOT X11_Xscreensaver_FOUND)
			target_compile_definitions(fte_platform_client INTERFACE NO_X11_XSS)
		endif()
	else()
		message(WARNING "X11 not available.")
		target_compile_definitions(fte_platform_client INTERFACE NO_X11)
	endif()

	find_library(FTE_WAYLAND_CLIENT NAMES wayland-client)
	find_library(FTE_WAYLAND_EGL NAMES wayland-egl)
	find_library(FTE_XKBCOMMON NAMES xkbcommon)
	if(FTE_WAYLAND_CLIENT AND FTE_WAYLAND_EGL AND FTE_XKBCOMMON)
		target_compile_definitions(fte_platform_client INTERFACE WAYLANDQUAKE USE_EGL)
		list(APPEND FTE_CLIENT_ARCH_FILES ${_E}/gl/gl_vidwayland.c)
	elseif(NOT X11_FOUND)
		message(WARNING "Neither X11 nor Wayland available: no renderers supported!")
	endif()

	find_library(FTE_OSSAUDIO NAMES ossaudio)	#openbsd
	if(FTE_OSSAUDIO)
		target_link_libraries(fte_platform_client INTERFACE ${FTE_OSSAUDIO})
	endif()

	if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
		message(WARNING "BSD/macOS builds need epoll-shim; not wired up in this build yet.")
	endif()
endif()

unset(_E)
