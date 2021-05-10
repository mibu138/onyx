add_executable(hello-triangle hello-triangle.c)
add_executable(hello-ui hello-ui.c)

target_include_directories(hello-triangle PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_include_directories(hello-ui PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(hello-triangle ${LIBRARY_NAME})
target_link_libraries(hello-ui ${LIBRARY_NAME})

set(SHADER_TARGET_NAME example-shaders)
add_subdirectory(shaders)
add_dependencies(hello-triangle ${SHADER_TARGET_NAME})
add_dependencies(hello-ui ${SHADER_TARGET_NAME})
