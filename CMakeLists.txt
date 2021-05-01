cmake_minimum_required(VERSION 3.17)

project(Obsidian)

set(CMAKE_INSTALL_MESSAGE LAZY)

include_directories(thirdparty /usr/include/freetype2)

set(LIBRARY_NAME obsidian)

add_subdirectory(src)
