#QTV proxy server.

if(FTE_TOOL_QTV)
	add_executable(qtv
		netchan.c
		parse.c
		msg.c
		qw.c
		source.c
		bsp.c
		rcon.c
		relay.c
		mdfour.c
		crc.c
		control.c
		forward.c
		pmove.c
		menu.c
		httpsv.c
		libqtvc/glibc_sucks.c
		${FTE_ROOT}/engine/common/md5.c
		${FTE_ROOT}/engine/common/sha1.c
	)
	target_link_libraries(qtv PRIVATE fte_compile_options fte_deps_base)
	if(MSVC)
		#qtv.h only pulls in ws2tcpip.h when IPPROTO_IPV6 is a macro, which modern Windows SDKs made an enum.
		#It also uses alloca without including malloc.h, where msvc declares it as _alloca.
		target_compile_definitions(qtv PRIVATE socklen_t=int alloca=_alloca)
		target_compile_options(qtv PRIVATE /FImalloc.h)
	endif()
	if(WIN32)
		target_link_libraries(qtv PRIVATE ws2_32 winmm)
	else()
		target_link_libraries(qtv PRIVATE m)
	endif()
	fte_setup_target(qtv)
endif()
