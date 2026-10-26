#QuakeEd 3.2 (https://github.com/Lunaran/QuakeEd, GPLv2): standalone Win32 + OpenGL1 editor, moved in from references/quaked3.
#Third-party code, so it keeps its own warning level.

enable_language(RC)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

file(GLOB _quaked3_sources CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.h)
#Leftovers upstream no longer compiles (nothing references them).
list(FILTER _quaked3_sources EXCLUDE REGEX "/(build|CmdTransform|win_proj|WndView)\\.(cpp|h)$")
add_executable(quaked3 WIN32 ${_quaked3_sources} win_qe3.rc)

#No include dir on purpose: IO.h would shadow the CRT's <io.h> on case-insensitive filesystems.
#The original project force-included pre.h through its PCH; a few sources rely on it.
target_precompile_headers(quaked3 PRIVATE pre.h)
target_compile_definitions(quaked3 PRIVATE
	_CRT_SECURE_NO_WARNINGS
	_CRT_NONSTDC_NO_DEPRECATE
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	NOMINMAX
	GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(quaked3 PRIVATE
	GLEW::GLEW glm::glm OpenGL::GL
	comctl32 shlwapi)
if(MSVC)
	target_compile_options(quaked3 PRIVATE /W3 /utf-8 /MP)
endif()
set_target_properties(quaked3 PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON)

fte_setup_target(quaked3)
set_target_properties(quaked3 PROPERTIES FOLDER "Editor")
