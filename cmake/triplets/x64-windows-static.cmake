#Overlay of vcpkg's x64-windows-static: same settings, but the deps are built with the same MSVC as the engine.
#vcpkg picks the newest Visual Studio it finds, and static libs from a newer MSVC fail to link against an older STL
#(eg. unresolved __std_rotate in OpenAL32.lib). Keep this in sync with the "Visual Studio 17 2022" presets.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_PROVIDED_FORTRAN ON)
set(VCPKG_PLATFORM_TOOLSET v143)
