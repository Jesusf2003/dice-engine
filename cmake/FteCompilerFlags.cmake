#Compiler flags shared by every FTE target, exposed as the fte_compile_options INTERFACE target.

#Plugins need hidden visibility to avoid clashing with engine symbols (matches Windows semantics anyway).
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(FTE_LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT _fte_ipo_ok OUTPUT _fte_ipo_msg LANGUAGES C CXX)
	if(_fte_ipo_ok)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
		message(STATUS "FTE: using LTO for release configs.")
	else()
		message(STATUS "FTE: no LTO - not supported.")
	endif()
endif()

add_library(fte_compile_options INTERFACE)

target_compile_definitions(fte_compile_options INTERFACE
	_FILE_OFFSET_BITS=64
	CONFIG_FILE_NAME=${FTE_BUILD_CONFIG}
	${FTE_REVISION_DEFINES}
	$<$<CONFIG:Debug>:_DEBUG>
)

target_include_directories(fte_compile_options INTERFACE
	"${FTE_ROOT}/engine/common"
	"${FTE_ROOT}/engine/client"
	"${FTE_ROOT}/engine/qclib"
	"${FTE_ROOT}/engine/gl"
	"${FTE_ROOT}/engine/server"
	"${FTE_ROOT}/engine"
)

if(MSVC)
	target_compile_definitions(fte_compile_options INTERFACE
		_CRT_SECURE_NO_WARNINGS
		_CRT_NONSTDC_NO_WARNINGS
		_WINSOCK_DEPRECATED_NO_WARNINGS
	)
	target_compile_options(fte_compile_options INTERFACE
		$<$<COMPILE_LANGUAGE:C,CXX>:/MP>
		$<$<COMPILE_LANGUAGE:C,CXX>:/W3>
		#4244/4267/4305: implicit narrowing all over the quake codebase. 4996: posix names.
		$<$<COMPILE_LANGUAGE:C,CXX>:/wd4244>
		$<$<COMPILE_LANGUAGE:C,CXX>:/wd4267>
		$<$<COMPILE_LANGUAGE:C,CXX>:/wd4305>
		$<$<COMPILE_LANGUAGE:C,CXX>:/wd4996>
	)
	#image.c/sys_plugfte.c request their own prebuilt libs via #pragma comment(lib, "../libs/...").
	#vcpkg provides those libraries instead, so tell the linker to ignore the requests.
	foreach(_lib IN ITEMS libpng64.lib libpng.lib libjpeg64.lib jpeg.lib zlib64.lib)
		target_link_options(fte_compile_options INTERFACE "/NODEFAULTLIB:../libs/${_lib}")
	endforeach()
else()
	set(_fte_gnu_c_warnings
		-Wno-pointer-sign
		-Wno-unknown-pragmas
		-Wno-format-zero-length
		-Wno-strict-aliasing
		-Wpointer-arith
		-Wvla							#msvc doesn't support vla
		-Wdeclaration-after-statement	#msvc 6-era compatibility
	)
	if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
		list(APPEND _fte_gnu_c_warnings
			-Wstrict-prototypes
			-Wold-style-definition
			-Wmissing-parameter-type
			-Wold-style-declaration
			-Wformat-truncation=1
		)
	endif()
	target_compile_options(fte_compile_options INTERFACE
		$<$<COMPILE_LANGUAGE:C>:${_fte_gnu_c_warnings}>
		$<$<CONFIG:Debug>:-Wall>
		$<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:C>>:-Wno-error=cpp>
		$<$<AND:$<CONFIG:Debug>,$<NOT:$<PLATFORM_ID:Windows>>>:-fstack-protector-strong>
		$<$<BOOL:${FTE_WERROR}>:-Werror>
	)
	if(FTE_NATIVE_ARCH)
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64")
			target_compile_options(fte_compile_options INTERFACE -mcpu=native)
		else()
			target_compile_options(fte_compile_options INTERFACE -march=native)
		endif()
	endif()
endif()

include(TestBigEndian)
test_big_endian(_fte_big_endian)
if(_fte_big_endian)
	target_compile_definitions(fte_compile_options INTERFACE FTE_BIG_ENDIAN)
else()
	target_compile_definitions(fte_compile_options INTERFACE FTE_LITTLE_ENDIAN)
endif()
