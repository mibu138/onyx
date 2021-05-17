cmake_minimum_required(VERSION 3.17)

project(Obsidian VERSION 1.0.0)

#set(CMAKE_VERBOSE_MAKEFILE ON)

include(/home/michaelb/dev/cmake/shaders.cmake)
include(GNUInstallDirs)

add_subdirectory(cmake)

find_package(Hell REQUIRED FATAL_ERROR)
find_package(Vulkan 1.2.162 REQUIRED FATAL_ERROR)

add_subdirectory(src/lib)
add_subdirectory(src/examples)
