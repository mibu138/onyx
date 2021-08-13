if(NOT OBSIDIAN_SKIP_EXAMPLES)
if(WIN32)
    set(OSTAG "WIN32")
else()
    set(OSTAG "")
endif()

add_executable(hello-triangle ${OSTAG} hello-triangle.c)
target_link_libraries(hello-triangle PRIVATE Obsidian::Obsidian )
set_target_properties(hello-triangle PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (FREETYPE_FOUND)
add_executable(hello-ui ${OSTAG} hello-ui.c)
target_link_libraries(hello-ui PRIVATE Obsidian::Obsidian )
set_target_properties(hello-ui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

add_subdirectory(shaders)
endif()
