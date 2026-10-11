#Third-party libraries, resolved through vcpkg.
#
#FTE can either link a library directly (FOO_STATIC style defines) or dlopen it at runtime.
#Every library found here is linked directly; disabling an FTE_DEP_* option falls back to the
#upstream behaviour (feature disabled via NO_*, or loaded at runtime for opus/speex/openal).
#
#Produces these INTERFACE targets:
#  fte_deps_base    - zlib/bzip2, used by everything that touches archives (engine, server, tools).
#  fte_deps_client  - image/font/audio/voice/renderer-header libraries used by client builds.

option(FTE_DEP_ZLIB      "Link against zlib."                                  ON)
option(FTE_DEP_BZIP2     "Link against libbzip2 (bz2-compressed pk3s)."        ON)
option(FTE_DEP_PNG       "Link against libpng."                                ON)
option(FTE_DEP_JPEG      "Link against libjpeg(-turbo)."                       ON)
option(FTE_DEP_FREETYPE  "Link against freetype."                              ON)
option(FTE_DEP_VORBIS    "Link against libvorbisfile."                         ON)
option(FTE_DEP_OPUS      "Link against opus (otherwise loaded at runtime)."    ON)
option(FTE_DEP_SPEEX     "Link against speex/speexdsp (otherwise runtime)."    ON)
option(FTE_DEP_VULKAN    "Build the Vulkan renderer (headers only)."           ON)
if(WIN32)
	option(FTE_DEP_OPENAL "Link against openal-soft (otherwise OpenAL32.dll is loaded at runtime)." ON)
else()
	option(FTE_DEP_OPENAL "Link against openal-soft (otherwise libopenal is loaded at runtime)." OFF)
endif()

add_library(fte_deps_base INTERFACE)
add_library(fte_deps_client INTERFACE)

#Finds a plain library (no CMake config shipped by its vcpkg port) and wraps it in an IMPORTED target,
#picking vcpkg's debug build for the Debug config so the CRTs match.
function(fte_find_plain_library target header)
	cmake_parse_arguments(A "" "" "NAMES;DEBUG_NAMES" ${ARGN})
	if(TARGET ${target})
		return()
	endif()
	string(MAKE_C_IDENTIFIER "${target}" _id)

	find_path(${_id}_INCLUDE_DIR "${header}")
	find_library(${_id}_LIBRARY_RELEASE NAMES ${A_NAMES} NAMES_PER_DIR)
	if(DEFINED VCPKG_INSTALLED_DIR)
		find_library(${_id}_LIBRARY_DEBUG NAMES ${A_DEBUG_NAMES} ${A_NAMES} NAMES_PER_DIR
			PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib" NO_DEFAULT_PATH)
	endif()

	if(NOT ${_id}_INCLUDE_DIR OR NOT ${_id}_LIBRARY_RELEASE)
		set(${_id}_FOUND FALSE PARENT_SCOPE)
		return()
	endif()

	add_library(${target} UNKNOWN IMPORTED GLOBAL)
	set_target_properties(${target} PROPERTIES
		IMPORTED_LOCATION "${${_id}_LIBRARY_RELEASE}"
		INTERFACE_INCLUDE_DIRECTORIES "${${_id}_INCLUDE_DIR}")
	if(${_id}_LIBRARY_DEBUG)
		set_property(TARGET ${target} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
		set_target_properties(${target} PROPERTIES IMPORTED_LOCATION_DEBUG "${${_id}_LIBRARY_DEBUG}")
	endif()
	set(${_id}_FOUND TRUE PARENT_SCOPE)
endfunction()

#--- zlib -------------------------------------------------------------------
if(FTE_DEP_ZLIB)
	find_package(ZLIB REQUIRED)
	target_link_libraries(fte_deps_base INTERFACE ZLIB::ZLIB)
	target_compile_definitions(fte_deps_base INTERFACE ZLIB_STATIC)
else()
	target_compile_definitions(fte_deps_base INTERFACE NO_ZLIB)
endif()

#--- bzip2 ------------------------------------------------------------------
if(FTE_DEP_BZIP2)
	find_package(BZip2 REQUIRED)
	target_link_libraries(fte_deps_base INTERFACE BZip2::BZip2)
	target_compile_definitions(fte_deps_base INTERFACE AVAIL_BZLIB BZLIB_STATIC)
endif()

target_link_libraries(fte_deps_client INTERFACE fte_deps_base)

#--- libpng -----------------------------------------------------------------
if(FTE_DEP_PNG)
	find_package(PNG REQUIRED)
	target_link_libraries(fte_deps_client INTERFACE PNG::PNG)
	target_compile_definitions(fte_deps_client INTERFACE LIBPNG_STATIC)
else()
	target_compile_definitions(fte_deps_client INTERFACE NO_PNG)
endif()

#--- libjpeg ----------------------------------------------------------------
if(FTE_DEP_JPEG)
	find_package(JPEG REQUIRED)
	target_link_libraries(fte_deps_client INTERFACE JPEG::JPEG)
	target_compile_definitions(fte_deps_client INTERFACE LIBJPEG_STATIC)
else()
	target_compile_definitions(fte_deps_client INTERFACE NO_JPEG)
endif()

#--- freetype ---------------------------------------------------------------
#Note: the engine checks FREETYPE_STATIC (upstream's cmake defined LIBFREETYPE_STATIC, which nothing reads).
if(FTE_DEP_FREETYPE)
	find_package(Freetype REQUIRED)
	target_link_libraries(fte_deps_client INTERFACE Freetype::Freetype)
	target_compile_definitions(fte_deps_client INTERFACE FREETYPE_STATIC)
	if(NOT WIN32)
		find_package(Fontconfig)
		if(Fontconfig_FOUND)
			target_link_libraries(fte_deps_client INTERFACE Fontconfig::Fontconfig)
			target_compile_definitions(fte_deps_client INTERFACE LIBFONTCONFIG_STATIC)
		endif()
	endif()
else()
	target_compile_definitions(fte_deps_client INTERFACE NO_FREETYPE)
endif()

#--- ogg/vorbis -------------------------------------------------------------
if(FTE_DEP_VORBIS)
	find_package(Vorbis CONFIG REQUIRED)
	target_link_libraries(fte_deps_client INTERFACE Vorbis::vorbisfile)
	target_compile_definitions(fte_deps_client INTERFACE LIBVORBISFILE_STATIC)
else()
	target_compile_definitions(fte_deps_client INTERFACE NO_OGG)
endif()

#--- opus (voice chat) ------------------------------------------------------
if(FTE_DEP_OPUS)
	find_package(Opus CONFIG REQUIRED)
	target_link_libraries(fte_deps_client INTERFACE Opus::opus)
	target_compile_definitions(fte_deps_client INTERFACE OPUS_STATIC)
endif()

#--- speex + speexdsp (voice chat, legacy codec + preprocessor) -------------
if(FTE_DEP_SPEEX)
	fte_find_plain_library(fte::speex    speex/speex.h            NAMES speex libspeex)
	fte_find_plain_library(fte::speexdsp speex/speex_preprocess.h NAMES speexdsp libspeexdsp)
	if(NOT TARGET fte::speex OR NOT TARGET fte::speexdsp)
		message(FATAL_ERROR "speex/speexdsp not found (they are listed in vcpkg.json).")
	endif()
	target_link_libraries(fte_deps_client INTERFACE fte::speex fte::speexdsp)
	target_compile_definitions(fte_deps_client INTERFACE SPEEX_STATIC)
endif()

#--- OpenAL -----------------------------------------------------------------
if(FTE_DEP_OPENAL)
	find_package(OpenAL CONFIG REQUIRED)
	target_link_libraries(fte_deps_client INTERFACE OpenAL::OpenAL)
	target_compile_definitions(fte_deps_client INTERFACE OPENAL_STATIC)
endif()

#--- Vulkan (headers only, the loader is opened at runtime) -----------------
if(FTE_DEP_VULKAN)
	find_package(VulkanHeaders CONFIG REQUIRED)
	target_link_libraries(fte_deps_client INTERFACE Vulkan::Headers)
	target_compile_definitions(fte_deps_client INTERFACE VKQUAKE)
endif()

#--- OpenGL (no import lib needed: the engine loads opengl32/libGL itself) --
if(WIN32)
	set(OpenGL_FOUND TRUE)
else()
	set(OpenGL_GL_PREFERENCE LEGACY)
	find_package(OpenGL)
endif()
if(OpenGL_FOUND)
	target_compile_definitions(fte_deps_client INTERFACE GLQUAKE)
else()
	message(WARNING "OpenGL headers not available, the engine will depend on Vulkan.")
	target_compile_definitions(fte_deps_client INTERFACE NO_OPENGL)
endif()
