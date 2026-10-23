#QuakeC toolchain: fteqcc (cli), fteqccgui (win32 gui), fteqccgui-imgui (Dear ImGui gui), qcvm (standalone vm).

if(FTE_TOOL_QCC)
	add_executable(fteqcc
		qcctui.c
		${FTE_QCC_FILES}
	)
	target_link_libraries(fteqcc PRIVATE fte_compile_options fte_deps_base)
	if(NOT WIN32)
		target_link_libraries(fteqcc PRIVATE m)
	endif()
	fte_setup_target(fteqcc)
endif()

if(FTE_TOOL_QCCGUI)
	if(WIN32)
		add_executable(fteqccgui WIN32
			qccgui.c
			qccguistuff.c
			${FTE_QCC_FILES}
		)
		target_link_libraries(fteqccgui PRIVATE fte_compile_options fte_deps_base shlwapi ole32 comctl32 comdlg32)
		fte_setup_target(fteqccgui)
	else()
		message(STATUS "FTE: fteqccgui (Qt5/QScintilla) is not ported to this build yet, skipping.")
	endif()
endif()

#Dear ImGui port of the Qt frontend (qccguiqt.cpp). Win32 + OpenGL2 backends for now.
if(FTE_TOOL_QCCGUI_IMGUI AND WIN32)
	find_package(imgui CONFIG REQUIRED)
	add_executable(fteqccgui-imgui WIN32
		qccguiimgui.cpp
		qccguistuff.c
		${FTE_QCC_FILES}
	)
	target_link_libraries(fteqccgui-imgui PRIVATE fte_compile_options fte_deps_base imgui::imgui opengl32 comdlg32)
	fte_setup_target(fteqccgui-imgui)
endif()

if(FTE_TOOL_QCVM)
	add_executable(qcvm
		test.c
		hash.c
		${FTE_QCVM_FILES}
	)
	target_link_libraries(qcvm PRIVATE fte_compile_options fte_deps_base)
	if(NOT WIN32)
		target_link_libraries(qcvm PRIVATE m)
	endif()
	fte_setup_target(qcvm)
endif()
