cmake_minimum_required(VERSION 3.17)

project(Obsidian VERSION 1.0)

include(GNUInstallDirs)

find_package(Hell REQUIRED FATAL_ERROR)
find_package(Vulkan 1.2.162 REQUIRED FATAL_ERROR)

add_subdirectory(cmake)
add_subdirectory(src/lib)
# Add an empty OpenEXRTargets.cmake file for the config to use. 
# Can be empty since we already defined the targets in add_subdirectory
#file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake/OpenEXRTargets.cmake" "# Dummy file")
add_subdirectory(src/examples)

