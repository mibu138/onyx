add_executable(hello-triangle hello-triangle.c)
add_executable(hello-ui hello-ui.c)

target_include_directories(hello-triangle PRIVATE "${CMAKE_SOURCE_DIR}/src/lib")
target_include_directories(hello-ui PRIVATE "${CMAKE_SOURCE_DIR}/src/lib")
target_link_libraries(hello-triangle Obsidian::Obsidian )
target_link_libraries(hello-ui Obsidian::Obsidian )

install(TARGETS hello-ui) #just a test

add_subdirectory(shaders)
