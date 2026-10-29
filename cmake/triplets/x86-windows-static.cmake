#Overlay of vcpkg's community x86-windows-static, pinned to the presets' MSVC (see x64-windows-static.cmake).
set(VCPKG_TARGET_ARCHITECTURE x86)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_PLATFORM_TOOLSET v143)
