cmake_minimum_required(VERSION 3.17)

project(Obsidian VERSION 1.0.0)

include(GNUInstallDirs)

find_package(Hell REQUIRED FATAL_ERROR)
find_package(Vulkan 1.2.162 REQUIRED FATAL_ERROR)

include(cmake/shaders.cmake)

add_subdirectory(cmake)
add_subdirectory(src/lib)
add_subdirectory(src/examples)
