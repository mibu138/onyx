cmake_minimum_required(VERSION 3.17)

project(Obsidian LANGUAGES C VERSION 0.1.0)

option(OBSIDIAN_SKIP_EXAMPLES "Skip building examples" OFF)
option(OBSIDIAN_SKIP_TESTS    "Skip building tests" OFF)

if(NOT DEFINED GIT_BASE_URL)
    set(GIT_BASE_URL https://github.com/mokchira)
endif()

include(FetchContent)
FetchContent_Declare(
    cmake-common
    GIT_REPOSITORY ${GIT_BASE_URL}/cmake-common
    GIT_TAG master)
FetchContent_GetProperties(cmake-common)
if(NOT cmake-common_POPULATED)
    FetchContent_Populate(cmake-common)
endif()

list(APPEND CMAKE_MODULE_PATH ${cmake-common_SOURCE_DIR})

FetchContent_Declare(
    hell 
    GIT_REPOSITORY ${GIT_BASE_URL}/hell
    GIT_TAG master)

FetchContent_Declare(
    coal
    GIT_REPOSITORY ${GIT_BASE_URL}/coal
    GIT_TAG master)

include(header)
set(CMAKE_BUILD_TYPE DEBUG)
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
find_package(Freetype)

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
