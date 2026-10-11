#Standalone tools built from engine sources: imgtool, iqmtool, httpserver, ftemaster.

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
