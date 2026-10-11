#Engine binaries: fteqw (client+server), fteqw-sv (dedicated server), fteqw-cl (client only).

if(FTE_ENGINE)
	add_executable(fteqw WIN32
		${FTE_CLIENT_ARCH_FILES}
		${FTE_COMMON_FILES}
		${FTE_CLIENT_FILES}
		${FTE_SERVER_FILES}
		${FTE_HEADER_FILES}
	)
	target_link_libraries(fteqw PRIVATE fte_compile_options fte_platform_client fte_deps_client)
	fte_setup_target(fteqw)

	#Translation template, only when gettext is around (upstream made this unconditional).
	find_program(FTE_XGETTEXT xgettext)
	if(FTE_XGETTEXT)
		add_custom_target(fteqw-i18n
			VERBATIM
			WORKING_DIRECTORY "${FTE_ROOT}"
			COMMAND "${FTE_XGETTEXT}" --output=${CMAKE_CURRENT_BINARY_DIR}/fteqw.pot -k -kCon_TPrintf -kCon_SafeTPrintf
				-kNetchan_OutOfBandTPrintf:4 -kSV_OutOfBandTPrintf:4 -klangtext -kSV_TPrintToClient:3 -kSV_ClientTPrintf:3
				-kSV_BroadcastTPrintf:2 -kCVARAFCD:6 -kCVARAFD:5 -kCVARFCD:5 -kCVARAD:4 -kCVARFD:4 -kCVARCD:4 -kCVARD:3
				-kCmd_AddCommandD:3 -kCmd_AddCommandAD:4 -kMenu_Prompt:4:5:6 -kHost_EndGame
				${FTE_CLIENT_ARCH_FILES} ${FTE_COMMON_FILES} ${FTE_CLIENT_FILES} ${FTE_SERVER_FILES}
			BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/fteqw.pot"
		)
	endif()
endif()

if(FTE_ENGINE_SERVER_ONLY)
	add_executable(fteqw-sv
		${FTE_SERVER_ARCH_FILES}
		${FTE_COMMON_FILES}
		${FTE_SERVER_FILES}
	)
	target_compile_definitions(fteqw-sv PRIVATE SERVERONLY)
	target_link_libraries(fteqw-sv PRIVATE fte_compile_options fte_platform_server fte_deps_base)
	fte_setup_target(fteqw-sv)
endif()

if(FTE_ENGINE_CLIENT_ONLY)
	add_executable(fteqw-cl WIN32
		${FTE_CLIENT_ARCH_FILES}
		${FTE_COMMON_FILES}
		${FTE_CLIENT_FILES}
	)
	target_compile_definitions(fteqw-cl PRIVATE CLIENTONLY)
	target_link_libraries(fteqw-cl PRIVATE fte_compile_options fte_platform_client fte_deps_client)
	fte_setup_target(fteqw-cl)
endif()
