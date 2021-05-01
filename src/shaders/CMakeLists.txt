set(SRCS
    default.frag
    default.vert
    full-screen.vert
    post.frag
    post.vert
    raytrace.rchit
    raytrace.rgen
    raytrace.rmiss
    raytraceShadow.rmiss
    ui-box.frag
    ui.frag
    ui-slider.frag
    ui-texture.frag
    ui.vert)

set(GLC glslc)
set(GLCFLAGS "--target-env=vulkan1.2")
set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

foreach(SRC ${SRCS})
    get_filename_component(FILE_NAME ${SRC} NAME)
    set(SPV "${PROJECT_BINARY_DIR}/src/shaders/${FILE_NAME}.spv")
    set(CMD ${GLC} ${GLCFLAGS} ${DIR}/${SRC} -o ${SPV})
    add_custom_command(
        OUTPUT ${SPV}
        COMMAND ${CMD}
        DEPENDS ${SRC})
    list(APPEND SPVS ${SPV})
endforeach(SRC)

if (NOT DEFINED SHADER_INSTALL_PREFIX)
    set(SHADER_INSTALL_PREFIX shaders/)
endif()

add_custom_target(shaders DEPENDS ${SPVS})

install(FILES ${SPVS}
    DESTINATION ${SHADER_INSTALL_PREFIX}${LIBRARY_NAME})

