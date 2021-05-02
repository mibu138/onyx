add_executable(hello-triangle hello-triangle.c)

target_include_directories(hello-triangle PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(hello-triangle ${LIBRARY_NAME})

set(SHADER_TARGET_NAME example-shaders)
add_subdirectory(shaders)
add_dependencies(hello-triangle ${SHADER_TARGET_NAME})
