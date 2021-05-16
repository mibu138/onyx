cmake_minimum_required(VERSION 3.17)

project(obsidian VERSION 1.0)

add_subdirectory(cmake)
add_subdirectory(src)
# Add an empty OpenEXRTargets.cmake file for the config to use. 
# Can be empty since we already defined the targets in add_subdirectory
#file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake/OpenEXRTargets.cmake" "# Dummy file")
add_subdirectory(examples)

