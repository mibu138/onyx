cmake_minimum_required(VERSION 3.17)

project(Obsidian LANGUAGES C VERSION 0.1.0)

option(OBSIDIAN_SKIP_EXAMPLES "Skip building examples" OFF)
option(OBSIDIAN_SKIP_TESTS    "Skip building tests" OFF)
option(OBSIDIAN_BUILD_GLSLC   "Build glslc executable" OFF)

if(NOT DEFINED HELL_URL)
    set(HELL_URL https://github.com/mokchira/hell)
endif()

if(NOT DEFINED COAL_URL)
    set(COAL_URL https://github.com/mokchira/coal)
endif()

include(FetchContent)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

FetchContent_Declare(
    hell 
    GIT_REPOSITORY ${HELL_URL}
    GIT_TAG master)

FetchContent_Declare(
    coal
    GIT_REPOSITORY ${COAL_URL}
    GIT_TAG master)

include(GNUInstallDirs)
enable_testing()

find_package(Vulkan 1.2.162)
if (NOT Vulkan_FOUND)
    FetchContent_Declare(
        vulkan_headers
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers
        GIT_TAG c5b7a2fa18750e435e91e06a50cdc5451c5b9abd)

    FetchContent_Declare(
        vulkan_loader
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader
        GIT_TAG 90fd66f60fa7de10c91030f8c88b2a5c7c377784)
    FetchContent_MakeAvailable(vulkan_headers vulkan_loader)
endif()

if (NOT DEFINED GLC)
    find_program(GLSLC glslc)
    if (GLSLC STREQUAL "GLSLC-NOTFOUND")
        if (OBSIDIAN_BUILD_GLSLC)
            FetchContent_Declare(
                glslang
                GIT_REPOSITORY https://github.com/KhronosGroup/glslang
                GIT_TAG c94ec9356f1a12876ae959409ec67eddb8a90c16)
            set(SKIP_GLSLANG_INSTALL ON)

            FetchContent_Declare(
                spirv_headers
                GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
                GIT_TAG e7b49d7fb59808a650618e0a4008d4bae927e112)
            set(SPIRV_HEADERS_SKIP_EXAMPLES ON)
            set(SPIRV_HEADERS_SKIP_INSTALL ON)

            FetchContent_Declare(
                spirv_tools 
                GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
                GIT_TAG 5737dbb068da91274de9728aab8b4bf27c52f38d)
            set(SPIRV_BUILD_FUZZER OFF)
            set(SKIP_SPIRV_TOOLS_INSTALL ON)
            set(SPIRV_COLOR_TERMINAL OFF)
            set(SPIRV_SKIP_TESTS ON)
            set(SPIRV_SKIP_EXECUTABLES ON)

            FetchContent_Declare(
                shaderc 
                GIT_REPOSITORY https://github.com/google/shaderc
                GIT_TAG fadb0edb247a1daa74f9a206a27e9a1c0417ce49)
            set(SHADERC_SKIP_TESTS ON)
            set(SHADERC_SKIP_EXAMPLES ON)
            set(SHADERC_SKIP_INSTALL ON)

            FetchContent_MakeAvailable(glslang spirv_headers spirv_tools shaderc)
        else()
            message(FATAL_ERROR "Cannot find glslc. Set OBSIDIAN_BUILD_GLSLC=ON to build it.")
        endif()
    else()
        message("Using system glslc ${GLSLC}")
        add_executable(glslc_exe IMPORTED GLOBAL)
        set_target_properties(glslc_exe IMPORTED_LOCATION ${GLSLC})
    endif()
    set(GLC glslc_exe)
endif()

find_package(Freetype)

add_executable(Obsidian::glslc ALIAS glslc_exe)

FetchContent_MakeAvailable(hell coal)
add_library(thirdparty INTERFACE)
target_include_directories(thirdparty INTERFACE "thirdparty")

add_subdirectory(src/lib)
add_subdirectory(src/shaders)
if(NOT ${OBSIDIAN_SKIP_EXAMPLES})
    add_subdirectory(src/examples)
endif()
if(NOT ${OBSIDIAN_SKIP_TESTS})
    add_subdirectory(src/tests)
endif()
