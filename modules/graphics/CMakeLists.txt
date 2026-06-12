# @cp/graphics �?CP语言 图形模块（Raylib 2D/3D + ImGui�?cmake_minimum_required(VERSION 3.20)
project(cplang_graphics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cplang_graphics STATIC
    graphics.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_raylib.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_imgui.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_raylib_unit.cpp
    ${CMAKE_SOURCE_DIR}/src/stdlib/stdlib_raylib_stubs.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/rlImGui.cpp
)

target_include_directories(cplang_graphics PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/raylib/src
    ${CMAKE_SOURCE_DIR}/third_party/raylib/src/external/glfw/include
    ${CMAKE_SOURCE_DIR}/third_party/imgui
)

target_compile_definitions(cplang_graphics PRIVATE
    GRAPHICS_API_OPENGL_21 PLATFORM_DESKTOP NO_FONT_AWESOME
)

target_compile_options(cplang_graphics PRIVATE /utf-8)
target_compile_definitions(cplang_graphics PRIVATE _CRT_SECURE_NO_WARNINGS _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH=1)
target_link_libraries(cplang_graphics PUBLIC cplang_vm cplang_platform)
