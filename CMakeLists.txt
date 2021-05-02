cmake_minimum_required(VERSION 3.17)

project(obsidian)

if (NOT DEFINED GLOBAL_CMAKE_PATH)
    include(/home/michaelb/dev/cmake/global.cmake)
else()
    include({GLOBAL_CMAKE_PATH})
endif()

include_directories(thirdparty /usr/include/freetype2)

set(LIBRARY_NAME ${CMAKE_PROJECT_NAME})

add_subdirectory(src)
add_subdirectory(examples)

