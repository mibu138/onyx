cmake_minimum_required(VERSION 3.17)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/cmake-common)

project(Obsidian VERSION 0.1.0)

include(header)

find_package(Hell REQUIRED FATAL_ERROR)
find_package(Coal 0.2.0 REQUIRED FATAL_ERROR)
find_package(Vulkan 1.2.162 REQUIRED FATAL_ERROR)

add_subdirectory(cmake)
add_subdirectory(src/lib)
add_subdirectory(src/examples)
