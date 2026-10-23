cmake_minimum_required(VERSION 3.16)
project(imgui CXX)

set(CMAKE_DEBUG_POSTFIX d)

add_library(${PROJECT_NAME} "")
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        $<INSTALL_INTERFACE:include>
)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

set(IMGUI_INSTALL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
)

if(IMGUI_BUILD_OPENGL2_BINDING)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp)
    list(APPEND IMGUI_INSTALL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl2.h)
endif()

if(IMGUI_BUILD_OPENGL3_BINDING)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
    list(APPEND IMGUI_INSTALL_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3.h
        ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_opengl3_loader.h)
endif()

if(IMGUI_BUILD_SDL2_BINDING)
    find_package(SDL2 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp)
    list(APPEND IMGUI_INSTALL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_sdl2.h)
endif()

if(IMGUI_BUILD_WIN32_BINDING)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_win32.cpp)
    list(APPEND IMGUI_INSTALL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_win32.h)
endif()

if(IMGUI_FREETYPE)
    find_package(freetype CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC freetype)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp)
    target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_ENABLE_FREETYPE)
    list(APPEND IMGUI_INSTALL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/misc/freetype/imgui_freetype.h)
endif()

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_target
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(NOT IMGUI_SKIP_HEADERS)
    install(FILES ${IMGUI_INSTALL_HEADERS} DESTINATION include)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(imgui-config.cmake.in imgui-config.cmake INSTALL_DESTINATION share/imgui)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/imgui-config.cmake DESTINATION share/imgui)

install(
    EXPORT ${PROJECT_NAME}_target
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_NAME}-targets.cmake
    DESTINATION share/${PROJECT_NAME}
)
