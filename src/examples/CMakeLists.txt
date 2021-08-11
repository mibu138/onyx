add_executable(hello-triangle hello-triangle.c)
target_link_libraries(hello-triangle PRIVATE Obsidian::Obsidian )
set_target_properties(hello-triangle PROPERTIES WIN32_EXECUTABLE TRUE)

if (FREETYPE_FOUND)
add_executable(hello-ui hello-ui.c)
target_link_libraries(hello-ui PRIVATE Obsidian::Obsidian )
endif()

add_subdirectory(shaders)
