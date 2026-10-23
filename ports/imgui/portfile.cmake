#Overlay port: the vcpkg baseline ships an older imgui, this one is pinned to the docking branch tag.
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ocornut/imgui
    REF "v${VERSION}-docking"
    SHA512 00ade7a3e2c2e34465f1bb9fafd78733d3e659617b0857cf1b3c7f04cfc1d8566d1fd6a8697946f0de7c808cce1c4d0a4b2195c7bc20144cfd8b1e9b1cfba621
    HEAD_REF docking
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/imgui-config.cmake.in" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
    freetype                    IMGUI_FREETYPE
    opengl2-binding             IMGUI_BUILD_OPENGL2_BINDING
    opengl3-binding             IMGUI_BUILD_OPENGL3_BINDING
    sdl2-binding                IMGUI_BUILD_SDL2_BINDING
    win32-binding               IMGUI_BUILD_WIN32_BINDING
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
    OPTIONS_DEBUG
        -DIMGUI_SKIP_HEADERS=ON
)

vcpkg_cmake_install()

if("freetype" IN_LIST FEATURES)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/imconfig.h" "//#define IMGUI_ENABLE_FREETYPE\n" "#define IMGUI_ENABLE_FREETYPE\n")
endif()

vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
