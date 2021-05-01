set(SRCS 
    v_video.c      
    v_memory.c     
	v_image.c      
	v_command.c    
	v_loader.c     
    v_swapchain.c  
    r_render.c     
    r_pipeline.c   
	r_renderpass.c 
	r_raytrace.c   
	r_geo.c        
	s_scene.c      
	f_file.c       
	u_ui.c         
	t_text.c       
	common.c       
	locations.c)

set(PUBLIC_HEADERS
    common.h
    dtags.h
    f_file.h
    f_prim.h
    locations.h
    max_prims.h
    r_api.h
    r_attribute.h
    r_geo.h
    r_pipeline.h
    r_raytrace.h
    r_render.h
    r_renderpass.h
    s_scene.h
    t_text.h
    types.h
    u_ui.h
    v_command.h
    v_def.h
    v_image.h
    v_memory.h
    v_swapchain.h
    v_video.h
    v_vulkan.h)

add_library(${LIBRARY_NAME} SHARED ${SRCS})
set_target_properties(obsidian PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
target_link_libraries(${LIBRARY_NAME} hell freetype vulkan)

add_subdirectory(shaders)
add_dependencies(obsidian shaders)

if (NOT DEFINED LIB_INSTALL_PREFIX)
    set(LIB_INSTALL_PREFIX lib)
endif()

install(TARGETS ${LIBRARY_NAME} 
    LIBRARY DESTINATION ${LIB_INSTALL_PREFIX})

message("Obsidian: library prefix: ${LIB_INSTALL_PREFIX}")

if (NOT ${HEADER_INSTALL_PREFIX})
    set(HEADER_INSTALL_PREFIX include/)
endif()

if (NOT DEFINED NO_HEADER_INSTALL)
    install(TARGETS ${LIBRARY_NAME} 
        PUBLIC_HEADER DESTINATION ${HEADER_INSTALL_PREFIX}${LIBRARY_NAME})
endif()
