#Top-level build for the FTEQW-based engine.
#Dependencies are resolved through vcpkg (see vcpkg.json / CMakePresets.json).
#The original upstream script is kept as CMakeLists.upstream.txt for reference.
cmake_minimum_required(VERSION 3.20)

#vcpkg integration: the presets pass the toolchain explicitly, this is only a fallback for plain `cmake -S . -B build`.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VCPKG_ROOT})
		set(_fte_vcpkg_root "$ENV{VCPKG_ROOT}")
	else()
		set(_fte_vcpkg_root "C:/vcpkg")
	endif()
	if(EXISTS "${_fte_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
		set(CMAKE_TOOLCHAIN_FILE "${_fte_vcpkg_root}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain")
	endif()
endif()
if(CMAKE_HOST_WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET)
	set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg triplet")
endif()

#Must be decided before project(), since vcpkg installs the manifest during project().
option(FTE_USE_SDL "Use the SDL2 platform layer instead of the native Win32/X11 one." OFF)
if(FTE_USE_SDL)
	list(APPEND VCPKG_MANIFEST_FEATURES "sdl2")
endif()

project(FTEQuake LANGUAGES C CXX)
if(WIN32)
	enable_language(RC)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(FTE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

#---------------------------------------------------------------------------
#Output layout: every binary lands directly in <repo>/bin.
#Per-config variants are set too, otherwise multi-config generators (Visual Studio) append /Debug, /Release.
#Debug builds get a _d suffix so they can coexist with release builds.
set(FTE_OUTPUT_DIR "${FTE_ROOT}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FTE_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${FTE_OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${FTE_OUTPUT_DIR}")
foreach(_cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
	string(TOUPPER "${_cfg}" _CFG)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${FTE_OUTPUT_DIR}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_CFG} "${FTE_OUTPUT_DIR}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_CFG} "${FTE_OUTPUT_DIR}")
endforeach()
set(CMAKE_DEBUG_POSTFIX "_d")

#Static vcpkg triplets are built against the static CRT, so match it.
if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "-static$")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

#---------------------------------------------------------------------------
#Build options.
set(FTE_BUILD_CONFIG "config_fteqw.h" CACHE STRING "Feature config header from engine/common (config_fteqw.h, config_minimal.h, ...).")

option(FTE_ENGINE              "Build fteqw (client+server)."            ON)
option(FTE_ENGINE_SERVER_ONLY  "Build fteqw-sv (dedicated server)."      ON)
option(FTE_ENGINE_CLIENT_ONLY  "Build fteqw-cl (client only)."           OFF)
option(FTE_TOOL_QCC            "Build fteqcc (commandline QC compiler)." ON)
option(FTE_TOOL_QCCGUI         "Build fteqccgui (Windows only)."         ${WIN32})
option(FTE_TOOL_QCVM           "Build the standalone qcvm."              OFF)
option(FTE_TOOL_IMAGE          "Build imgtool."                          ON)
option(FTE_TOOL_IQM            "Build iqmtool."                          ON)
option(FTE_TOOL_QTV            "Build the qtv proxy."                    ON)
if(WIN32)
	#Neither builds on win32 upstream: httpserver lacks Sys_mkdir/remove/Rename outside the engine.
	option(FTE_TOOL_HTTPSV     "Build the small http server."            OFF)
	option(FTE_TOOL_MASTER     "Build the master server."                OFF)
else()
	option(FTE_TOOL_HTTPSV     "Build the small http server."            ON)
	option(FTE_TOOL_MASTER     "Build the master server."                ON)
endif()

option(FTE_WERROR              "Treat warnings as errors (gcc/clang)."   OFF)
option(FTE_NATIVE_ARCH         "Optimise for the build machine (-march=native, non-portable)." OFF)
option(FTE_LTO                 "Use link-time optimisation for non-debug configs." ON)

include(FteRevision)
include(FteCompilerFlags)
include(FteDependencies)
include(FteSources)
include(FtePlatform)

#Windows binaries follow the Makefile naming: 64bit builds get a "64" suffix (fteqw64.exe), 32bit ones none (fteqw.exe),
#so both architectures can share bin/.
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(FTE_BITS_SUFFIX "64")
else()
	set(FTE_BITS_SUFFIX "")
endif()

#Applies the shared output conventions to a target.
function(fte_setup_target target)
	set_target_properties(${target} PROPERTIES
		OUTPUT_NAME "${target}${FTE_BITS_SUFFIX}"
		DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
		FOLDER "FTE"
	)
	#Let 32bit builds address beyond 2GB, like upstream's win32 builds.
	if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
		target_link_options(${target} PRIVATE /LARGEADDRESSAWARE)
	endif()
endfunction()

add_subdirectory(engine)
add_subdirectory(engine/qclib)
add_subdirectory(fteqtv)
add_subdirectory(tools)

if(FTE_ENGINE)
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT fteqw)
endif()

message(STATUS "FTE: platform layer      = ${FTE_PLATFORM}")
message(STATUS "FTE: vcpkg triplet       = ${VCPKG_TARGET_TRIPLET}")
message(STATUS "FTE: binaries output dir = ${FTE_OUTPUT_DIR}")
