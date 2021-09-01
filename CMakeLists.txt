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
find_package(Vulkan 1.2.162 REQUIRED FATAL_ERROR)
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
