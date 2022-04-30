list(APPEND SRCS
    video.c
    memory.c
    image.c
    command.c
    loader.c
    swapchain.c
    render.c
    frame.c
    pipeline.c
    renderpass.c
    raytrace.c
    geo.c
    scene.c
    file.c
    common.c
    util.c
    locations.c
    mikktspace.c
    )
list(APPEND DEPS
    Vulkan::Vulkan
    Hell::Hell
    Coal::Coal
    )
#private. users should set their own versions for these and not rely on ours.
list(APPEND PRIVATE_DEPS
    )
if(FREETYPE_FOUND)
    list(APPEND SRCS
	ui.c
	text.c)
    list(APPEND DEPS
    Freetype::Freetype)
    set(FT2_DIR ${FREETYPE_INCLUDE_DIR_ft2build})
endif()

if(${ONYX_ENABLE_SHADERC})
    list(APPEND PRIVATE_DEPS shaderc)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(onyx STATIC)
target_sources(onyx PRIVATE ${SRCS})
target_include_directories(onyx
    PRIVATE "../include/onyx"
    INTERFACE "../include")
target_link_libraries(onyx
    PRIVATE ${PRIVATE_DEPS}
    PUBLIC ${DEPS})

if (${ONYX_ENABLE_GLSLANG})
    add_library(glslang_wrapper STATIC
        glslang-wrapper.cpp)
    target_link_libraries(glslang_wrapper 
        PRIVATE SPIRV glslang Hell::Hell)
    target_link_libraries(onyx
        PRIVATE glslang_wrapper)
    target_compile_definitions(onyx PUBLIC ONYX_GLSLANG_ENABLED)
endif()

if (${ONYX_ENABLE_SHADERC})
target_compile_definitions(onyx PUBLIC ONYX_SHADERC_ENABLED)
endif()
if (ONYX_NO_BUFFER_DEVICE_ADDRESS)
target_compile_definitions(onyx PUBLIC ONYX_NO_BUFFER_DEVICE_ADDRESS)
endif()
if(WIN32)
set_target_properties(onyx PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()
add_library(Onyx::Onyx ALIAS onyx)

if(UNIX)
    target_compile_definitions(onyx PUBLIC UNIX)
    target_compile_options(onyx PUBLIC -Wno-missing-braces -Wno-error
        -fdiagnostics-color=always)
elseif(WIN32)
    target_compile_definitions(onyx PUBLIC WIN32)
else()
    message(FATAL_ERROR "Need additional platform checks if not on unix.")
endif()

add_subdirectory(shaders)
