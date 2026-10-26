#Map editors, both built into <repo>/bin next to the engine:
#  radiant - the new editor: SDL2 + Dear ImGui (docking) + OpenGL, runs on any OS.
#  quaked3 - QuakeEd 3.2, plain Win32; kept building as the reference while it gets migrated into radiant.

if(FTE_EDITOR_RADIANT)
	add_subdirectory(radiant)
endif()

if(FTE_EDITOR_QUAKED3)
	if(WIN32)
		add_subdirectory(quaked3)
	else()
		message(STATUS "FTE: quaked3 is Win32 only, skipping.")
	endif()
endif()

if (FTE_COMPILER)

endif()