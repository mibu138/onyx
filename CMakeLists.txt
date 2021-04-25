cmake_minimum_required(VERSION 3.10)

project(Obsidian)

set(CMAKE_MODULE_PATH "/home/michaelb/dev/cmake")
include(global)

include_directories(thirdparty /usr/include/freetype2)
add_subdirectory(src)
