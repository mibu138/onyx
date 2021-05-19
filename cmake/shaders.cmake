macro(build_shaders)
    if (NOT DEFINED SRCS)
        message(FATAL_ERROR "SRCS must be defined as the list of shader source files to build.")
    endif()
    if (NOT DEFINED SHADER_TARGET_NAME)
        message(FATAL_ERROR "SHADER_TARGET_NAME must be defined as the name of this shader group target.")
    endif()
    set(GLC glslc)
    set(GLCFLAGS "--target-env=vulkan1.2")
    set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

    foreach(SRC ${SRCS})
        get_filename_component(FILE_NAME ${SRC} NAME)
        set(SPV "${PROJECT_BINARY_DIR}/src/lib/shaders/${FILE_NAME}.spv")
        set(CMD ${GLC} ${GLCFLAGS} ${DIR}/${SRC} -o ${SPV})
        add_custom_command(
            OUTPUT ${SPV}
            COMMAND ${CMD}
            DEPENDS ${SRC})
        list(APPEND SPVS ${SPV})
    endforeach(SRC)

    add_custom_target(${SHADER_TARGET_NAME} DEPENDS ${SPVS})
endmacro()

macro(install_shaders)
    message("Installing shaders")
    if (NOT DEFINED SHADER_INSTALL_PREFIX)
        set(SHADER_INSTALL_PREFIX ${CMAKE_INSTALL_DATADIR}/shaders)
    endif()
    if (NOT DEFINED SPVS)
        message(FATAL_ERROR "SPVS not defined. Can call build_shaders() before to create them. Aborting shader installation.")
    endif()
    if (DEFINED SHADER_INSTALL_DIR)
        message("Installing shaders to ${SHADER_INSTALL_DIR}")
        install(FILES ${SPVS}
            DESTINATION ${SHADER_INSTALL_DIR})
    else()
        install(FILES ${SPVS}
            DESTINATION ${SHADER_INSTALL_PREFIX}/obsidian)
    endif()
endmacro()
