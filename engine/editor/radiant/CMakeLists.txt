#radiant: the map editor. SDL2 window + Dear ImGui (docking branch, ports/imgui) + OpenGL3.
#QuakeEd 3 (engine/editor/quaked3) is the reference being migrated into it by hand.

find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(radiant
	main.cpp
)
target_link_libraries(radiant PRIVATE
	$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
	$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
	imgui::imgui
	OpenGL::GL
)
target_include_directories(radiant PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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
