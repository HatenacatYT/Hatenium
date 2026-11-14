cmake_minimum_required(VERSION 3.20)
project(Hatenium VERSION 0.1.0 LANGUAGES C CXX)

if(MSVC)
    message(FATAL_ERROR "Hatenium targets MinGW-w64 GCC (MSYS2 UCRT64). MSVC / Visual Studio is not supported.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(HATENIUM_STATIC_RUNTIME "Link the GCC/C++ runtime statically (single standalone exe)" ON)

# ---------------------------------------------------------------------------
# Dependencies (fetched from their official GitHub repositories, pinned)
# To build offline: -DFETCHCONTENT_SOURCE_DIR_IMGUI=<path> -DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=<path>
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE)

FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install    OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(imgui nlohmann_json)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(imgui_lib PUBLIC UNICODE _UNICODE NOMINMAX)
target_link_libraries(imgui_lib PUBLIC d3d11 dxgi d3dcompiler dwmapi imm32 gdi32 dxguid uuid)

# ---------------------------------------------------------------------------
# Hatenium
# ---------------------------------------------------------------------------
file(GLOB_RECURSE HATENIUM_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(Hatenium WIN32 ${HATENIUM_SOURCES})
target_include_directories(Hatenium PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(Hatenium PRIVATE
    HATENIUM_VERSION="${PROJECT_VERSION}"
    WINVER=0x0A00 _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN)
target_compile_options(Hatenium PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(Hatenium PRIVATE imgui_lib nlohmann_json::nlohmann_json comdlg32 user32 shell32 ole32)

if(HATENIUM_STATIC_RUNTIME)
    target_link_options(Hatenium PRIVATE -static -static-libgcc -static-libstdc++)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_options(Hatenium PRIVATE -s)
endif()

set_target_properties(Hatenium PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_command(TARGET Hatenium POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Hatenium>/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Hatenium>/assets
    COMMENT "Preparing config/ and assets/ next to Hatenium.exe")
