cmake_minimum_required(VERSION 3.17)

project(Obsidian VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(
    cmake-common
    GIT_REPOSITORY /home/michaelb/dev/cmake-common
    GIT_TAG origin/master)
FetchContent_GetProperties(cmake-common)
if(NOT cmake-common_POPULATED)
    FetchContent_Populate(cmake-common)
endif()

list(APPEND CMAKE_MODULE_PATH ${cmake-common_SOURCE_DIR})

set(git_base_url /home/michaelb/dev)

FetchContent_Declare(
    hell 
    GIT_REPOSITORY ${git_base_url}/hell
    GIT_TAG master)

FetchContent_Declare(
    coal
    GIT_REPOSITORY ${git_base_url}/coal
    GIT_TAG master)

find_package(Vulkan 1.2.162 REQUIRED FATAL_ERROR)
find_package(Freetype)

FetchContent_MakeAvailable(hell coal)
add_library(thirdparty INTERFACE)
target_include_directories(thirdparty INTERFACE "thirdparty")

add_subdirectory(src/lib)
add_subdirectory(src/shaders)
add_subdirectory(src/examples)
add_subdirectory(src/tests)
