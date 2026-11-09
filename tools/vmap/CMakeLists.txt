cmake_minimum_required(VERSION 3.27)
project(vmap LANGUAGES C)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#
# external dependencies
#

find_package(Math)

include(FetchContent)

# NOTE: deprecated library and should be replaced
find_package(LibXml2)
if(NOT LibXml2_FOUND)
	FetchContent_Declare(
		LibXml2
		GIT_REPOSITORY "https://gitlab.gnome.org/GNOME/libxml2.git"
		GIT_TAG "v2.15.4"
		EXCLUDE_FROM_ALL
		OVERRIDE_FIND_PACKAGE
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
	)
	set(LIBXML2_WITH_ICONV OFF CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(LibXml2)
endif()

# NOTE: only used for a handful of things and could be replaced
cmake_pkg_config(IMPORT glib-2.0 REQUIRED)

FetchContent_Declare(
	minizip-ng
	GIT_REPOSITORY "https://github.com/zlib-ng/minizip-ng.git"
	GIT_TAG "4.2.2"
	EXCLUDE_FROM_ALL
	OVERRIDE_FIND_PACKAGE
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
set(MZ_COMPAT ON CACHE BOOL "" FORCE)
set(MZ_OPENSSL OFF CACHE BOOL "" FORCE)
set(MZ_FETCH_LIBS ON CACHE BOOL "" FORCE)
set(MZ_FORCE_FETCH_LIBS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(minizip-ng)

#
# vendored libraries
#

add_library(vmap_ddslib
	STATIC
		${PROJECT_SOURCE_DIR}/libs/ddslib/ddslib.c
)
target_include_directories(vmap_ddslib
	PRIVATE
		${PROJECT_SOURCE_DIR}/libs/
)

add_library(vmap_etclib
	STATIC
		${PROJECT_SOURCE_DIR}/libs/etclib/etclib.c
)
target_include_directories(vmap_etclib
	PRIVATE
		${PROJECT_SOURCE_DIR}/libs/
)

add_library(vmap_filematch
	STATIC
		${PROJECT_SOURCE_DIR}/libs/filematch/filematch.c
)
target_include_directories(vmap_filematch
	PRIVATE
		${PROJECT_SOURCE_DIR}/libs/
)

add_library(vmap_l_net
	STATIC
		${PROJECT_SOURCE_DIR}/libs/l_net/l_net.c
		${PROJECT_SOURCE_DIR}/libs/l_net/l_net_berkley.c
		${PROJECT_SOURCE_DIR}/libs/l_net/l_net_wins.c
)
target_include_directories(vmap_l_net
	PRIVATE
		${PROJECT_SOURCE_DIR}/libs/
)

add_library(vmap_mathlib
	STATIC
		${PROJECT_SOURCE_DIR}/libs/mathlib/bbox.c
		${PROJECT_SOURCE_DIR}/libs/mathlib/line.c
		${PROJECT_SOURCE_DIR}/libs/mathlib/m4x4.c
		${PROJECT_SOURCE_DIR}/libs/mathlib/mathlib.c
		${PROJECT_SOURCE_DIR}/libs/mathlib/ray.c
)
target_include_directories(vmap_mathlib
	PRIVATE
		${PROJECT_SOURCE_DIR}/libs/
)

add_library(vmap_picomodel
	STATIC
		${PROJECT_SOURCE_DIR}/libs/picomodel/clip.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/envelope.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/list.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/lwio.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/lwo2.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/lwob.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/picointernal.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/picomodel.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/picomodules.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_3ds.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_ase.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_fm.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_iqm.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_lwo.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_md2.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_md3.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_mdc.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_ms3d.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_obj.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pm_terrain.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/pntspols.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/surface.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/vecmath.c
		${PROJECT_SOURCE_DIR}/libs/picomodel/vmap.c
)
target_include_directories(vmap_picomodel
	PRIVATE
		${PROJECT_SOURCE_DIR}/libs/
)

#
# main executable
#

add_executable(vmap
	${PROJECT_SOURCE_DIR}/common/cmdlib.c
	${PROJECT_SOURCE_DIR}/common/imagelib.c
	${PROJECT_SOURCE_DIR}/common/inout.c
	${PROJECT_SOURCE_DIR}/common/md4.c
	${PROJECT_SOURCE_DIR}/common/mutex.c
	${PROJECT_SOURCE_DIR}/common/polylib.c
	${PROJECT_SOURCE_DIR}/common/scriplib.c
	${PROJECT_SOURCE_DIR}/common/matlib.c
	${PROJECT_SOURCE_DIR}/common/threads.c
	${PROJECT_SOURCE_DIR}/common/vfs.c
	${PROJECT_SOURCE_DIR}/src/bsp/brush.c
	${PROJECT_SOURCE_DIR}/src/bsp/brush_primit.c
	${PROJECT_SOURCE_DIR}/src/bsp/bsp.c
	${PROJECT_SOURCE_DIR}/src/bsp_analyze.c
	${PROJECT_SOURCE_DIR}/src/bsp_info.c
	${PROJECT_SOURCE_DIR}/src/bsp_scale.c
	${PROJECT_SOURCE_DIR}/src/bspfile_abstract.c
	${PROJECT_SOURCE_DIR}/src/bspfile_ibsp.c
	${PROJECT_SOURCE_DIR}/src/bspfile_rbsp.c
	${PROJECT_SOURCE_DIR}/src/convert_ase.c
	${PROJECT_SOURCE_DIR}/src/convert_bsp.c
	${PROJECT_SOURCE_DIR}/src/convert_map.c
	${PROJECT_SOURCE_DIR}/src/convert_obj.c
	${PROJECT_SOURCE_DIR}/src/bsp/decals.c
	${PROJECT_SOURCE_DIR}/src/exportents.c
	${PROJECT_SOURCE_DIR}/src/bsp/facebsp.c
	${PROJECT_SOURCE_DIR}/src/fixaas.c
	${PROJECT_SOURCE_DIR}/src/bsp/fog.c
	${PROJECT_SOURCE_DIR}/src/help.c
	${PROJECT_SOURCE_DIR}/src/image.c
	${PROJECT_SOURCE_DIR}/src/bsp/leakfile.c
	${PROJECT_SOURCE_DIR}/src/light/light.c
	${PROJECT_SOURCE_DIR}/src/light/light_bounce.c
	${PROJECT_SOURCE_DIR}/src/light/light_trace.c
	${PROJECT_SOURCE_DIR}/src/light/light_ydnar.c
	${PROJECT_SOURCE_DIR}/src/light/lightmaps_ydnar.c
	${PROJECT_SOURCE_DIR}/src/main.c
	${PROJECT_SOURCE_DIR}/src/bsp/map.c
	${PROJECT_SOURCE_DIR}/src/bsp/mesh.c
	${PROJECT_SOURCE_DIR}/src/bsp/model.c
	${PROJECT_SOURCE_DIR}/src/bsp/patch.c
	${PROJECT_SOURCE_DIR}/src/path_init.c
	${PROJECT_SOURCE_DIR}/src/bsp/portals.c
	${PROJECT_SOURCE_DIR}/src/bsp/prtfile.c
	${PROJECT_SOURCE_DIR}/src/shaders.c
	${PROJECT_SOURCE_DIR}/src/bsp/surface.c
	${PROJECT_SOURCE_DIR}/src/bsp/surface_extra.c
	${PROJECT_SOURCE_DIR}/src/bsp/surface_foliage.c
	${PROJECT_SOURCE_DIR}/src/bsp/surface_fur.c
	${PROJECT_SOURCE_DIR}/src/bsp/surface_meta.c
	${PROJECT_SOURCE_DIR}/src/bsp/tjunction.c
	${PROJECT_SOURCE_DIR}/src/bsp/tree.c
	${PROJECT_SOURCE_DIR}/src/vis/vis.c
	${PROJECT_SOURCE_DIR}/src/vis/visflow.c
	${PROJECT_SOURCE_DIR}/src/bsp/writebsp.c
)
target_include_directories(vmap
	PRIVATE
		${PROJECT_SOURCE_DIR}/src/
		${PROJECT_SOURCE_DIR}/include/
		${PROJECT_SOURCE_DIR}/common/
		${PROJECT_SOURCE_DIR}/libs/
)
target_link_libraries(vmap
	PRIVATE
		$<TARGET_NAME_IF_EXISTS:Math::Math>
		MINIZIP::minizip
		PkgConfig::glib-2.0
		LibXml2::LibXml2
		vmap_ddslib
		vmap_etclib
		vmap_filematch
		vmap_l_net
		vmap_mathlib
		vmap_picomodel
)
