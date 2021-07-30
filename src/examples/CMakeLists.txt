add_executable(hello-triangle hello-triangle.c)
add_executable(hello-ui hello-ui.c)

target_link_libraries(hello-triangle PRIVATE Obsidian::Obsidian )
target_link_libraries(hello-ui PRIVATE Obsidian::Obsidian )

add_subdirectory(shaders)
