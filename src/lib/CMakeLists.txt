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
    obsidian.h
    common.h
    dtags.h
    f_file.h
    f_prim.h
    locations.h
    max_prims.h
    framebuffer.h
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

include(author_library)
author_library(obsidian 
    EXPORT_NAME Obsidian
    SOURCES ${SRCS}
    PUBLIC_HEADERS ${PUBLIC_HEADERS}
    DEPS Coal::Coal Hell::Hell Vulkan::Vulkan Freetype::Freetype
    EXTRA_INC_DIRS ${CMAKE_SOURCE_DIR}/thirdparty)
