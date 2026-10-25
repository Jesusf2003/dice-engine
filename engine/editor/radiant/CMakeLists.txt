#radiant: headless map editing core (brushes, faces, windings, .map io), extracted from OpenRadiant.
#No GUI, GLib or OpenGL dependencies; the ImGui editor links this and listens through DocumentHost.
#Can be built on its own (cmake -S engine/editor/radiant) or as part of the FTE tree.
cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(radiant LANGUAGES CXX)
	enable_testing()
	set(_radiant_toplevel ON)
else()
	set(_radiant_toplevel OFF)
endif()

option(RADIANT_BUILD_TESTS          "Build the radiant test executables (run them with ctest)." ON)
option(RADIANT_WARNINGS_AS_ERRORS   "Treat compiler warnings as errors in radiant."             ${_radiant_toplevel})

#Inside FTE every artifact goes to <repo>/bin; a static lib and its tests don't belong there.
foreach(_cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
	string(TOUPPER "${_cfg}" _CFG)
	unset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_CFG})
	unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_CFG})
endforeach()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

#Strict warnings, applied to radiant's own targets only.
function(radiant_set_warnings target)
	if(MSVC)
		target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
		if(RADIANT_WARNINGS_AS_ERRORS)
			target_compile_options(${target} PRIVATE /WX)
		endif()
	else()
		target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wold-style-cast)
		if(RADIANT_WARNINGS_AS_ERRORS)
			target_compile_options(${target} PRIVATE -Werror)
		endif()
	endif()
endfunction()

function(radiant_set_standard target)
	set_target_properties(${target} PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		FOLDER "Editor")
endfunction()

#---------------------------------------------------------------------------
add_library(radiant STATIC
	include/radiant/host.h
	include/radiant/math/vec3.h
	include/radiant/math/plane.h
	include/radiant/math/aabb.h
	include/radiant/geom/winding.h
	include/radiant/scene/face.h
	include/radiant/scene/brush.h

	src/geom/winding.cpp
	src/scene/brush.cpp
)
add_library(radiant::radiant ALIAS radiant)

target_include_directories(radiant
	PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(radiant PUBLIC cxx_std_17)
radiant_set_standard(radiant)
radiant_set_warnings(radiant)

#---------------------------------------------------------------------------
#Tests: plain executables using tests/check.h, registered with ctest.
if(RADIANT_BUILD_TESTS)
	function(radiant_add_test name)
		add_executable(${name} tests/${name}.cpp tests/check.h)
		target_link_libraries(${name} PRIVATE radiant::radiant)
		radiant_set_standard(${name})
		radiant_set_warnings(${name})
		add_test(NAME radiant.${name} COMMAND ${name})
	endfunction()

	radiant_add_test(map_roundtrip_test)
endif()
