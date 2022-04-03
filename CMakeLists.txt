cmake_minimum_required(VERSION 3.17)

project(Onyx VERSION 0.1.0)

option(ONYX_SKIP_EXAMPLES "Skip building examples" OFF)
option(ONYX_SKIP_TESTS    "Skip building tests" OFF)
option(ONYX_BUILD_GLSLC   "Build glslc executable" OFF)
option(ONYX_ENABLE_SHADERC   "Access shaderc functionality" OFF)
option(ONYX_BUILD_VULKAN_FROM_SOURCE "Access shaderc functionality" OFF)

if(NOT DEFINED HELL_URL)
    set(HELL_URL https://github.com/mokchira/hell)
endif()

if(NOT DEFINED COAL_URL)
    set(COAL_URL https://github.com/mokchira/coal)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(FetchContent)
include(fetch_shaderc)

FetchContent_Declare(
    hell
    GIT_REPOSITORY ${HELL_URL}
    GIT_TAG onyx)

FetchContent_Declare(
    coal
    GIT_REPOSITORY ${COAL_URL}
    GIT_TAG onyx)

include(GNUInstallDirs)
enable_testing()

if (${ONYX_ENABLE_SHADERC})
    fetch_shaderc()
    set(GLC glslc_exe)
endif()

if (NOT ONYX_BUILD_VULKAN_FROM_SOURCE)
    find_package(Vulkan 1.2.162)
    if (NOT Vulkan_FOUND)
        set(ONYX_BUILD_VULKAN_FROM_SOURCE TRUE)
    endif()
endif()

if (ONYX_BUILD_VULKAN_FROM_SOURCE)
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
        if (ONYX_BUILD_GLSLC AND NOT ${ONYX_ENABLE_SHADERC})
            fetch_shaderc()
        else()
            message(FATAL_ERROR "Cannot find glslc. Set ONYX_BUILD_GLSLC=ON to build it.")
        endif()
    else()
        message("Using system glslc ${GLSLC}")
        add_executable(glslc_exe IMPORTED GLOBAL)
        set_target_properties(glslc_exe PROPERTIES IMPORTED_LOCATION ${GLSLC})
    endif()
    set(GLC glslc_exe)
endif()

find_package(Freetype)

add_executable(Onyx::glslc ALIAS glslc_exe)

FetchContent_MakeAvailable(hell coal)

add_subdirectory(src)
if(NOT ${ONYX_SKIP_EXAMPLES})
    add_subdirectory(examples)
endif()
if(NOT ${ONYX_SKIP_TESTS})
    add_subdirectory(tests)
endif()


