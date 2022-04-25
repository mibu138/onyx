cmake_minimum_required(VERSION 3.17)

project(Onyx VERSION 0.1.0)

option(ONYX_SKIP_EXAMPLES "Skip building examples" OFF)
option(ONYX_SKIP_TESTS    "Skip building tests" OFF)
option(ONYX_BUILD_GLSLC   "Build glslc executable" OFF)
option(ONYX_ENABLE_SHADERC   "Access shaderc functionality" OFF)
option(ONYX_ENABLE_GLSLANG   "Access glslang functionality" ON)
option(ONYX_BUILD_VULKAN_FROM_SOURCE "Access shaderc functionality" ON)
option(ONYX_CREATE_DEP_SRC_SYMLINKS "Access shaderc functionality" ON)
option(ONYX_NO_BUFFER_DEVICE_ADDRESS "Disables buffer device address feature"
    OFF)

if(NOT DEFINED HELL_URL)
    set(HELL_URL https://github.com/mokchira/hell)
endif()

if(NOT DEFINED COAL_URL)
    set(COAL_URL https://github.com/mokchira/coal)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
include(fetch_shaderc)

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

if (${ONYX_ENABLE_SHADERC})
    fetch_shaderc()
    set(GLC glslc_exe)
endif()

if (${ONYX_ENABLE_GLSLANG})
    FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
        GIT_TAG master)
    set(BUILD_EXTERNAL OFF)
    set(ENABLE_WERROR OFF)
    message("Enabling glslang")
    FetchContent_MakeAvailable(glslang)
    set(GLC glslangValidator)
endif()

if (NOT ONYX_BUILD_VULKAN_FROM_SOURCE)
    find_package(Vulkan 1.3)
    if (NOT Vulkan_FOUND)
        set(ONYX_BUILD_VULKAN_FROM_SOURCE TRUE)
    endif()
endif()

if (ONYX_BUILD_VULKAN_FROM_SOURCE)
    FetchContent_Declare(
        vulkan_headers
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers
        GIT_TAG v1.3.212)

    FetchContent_Declare(
        vulkan_loader
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader
        GIT_TAG v1.3.212)
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

add_executable(Onyx::glslc ALIAS ${GLC})

FetchContent_MakeAvailable(hell coal)

add_subdirectory(src)
if(NOT ${ONYX_SKIP_EXAMPLES})
    add_subdirectory(examples)
endif()
if(NOT ${ONYX_SKIP_TESTS})
    add_subdirectory(tests)
endif()

file(CREATE_LINK _deps/vulkan_loader-src loader SYMBOLIC)
file(CREATE_LINK _deps/hell-src hell SYMBOLIC)
file(CREATE_LINK _deps/coal-src hell SYMBOLIC)
file(CREATE_LINK _deps/glslang-src glslang SYMBOLIC)
