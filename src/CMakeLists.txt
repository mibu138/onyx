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

add_library(obsidian SHARED ${SRCS})
set_target_properties(obsidian PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
target_link_libraries(obsidian hell freetype vulkan)
add_subdirectory(shaders)
add_dependencies(obsidian shaders)
install(TARGETS obsidian 
    LIBRARY DESTINATION lib 
    PUBLIC_HEADER DESTINATION include/obsidian)

install(TARGETS obsidian
    LIBRARY DESTINATION ${HOUDINI_TOOL_INS_DIR}/lib 
    PUBLIC_HEADER DESTINATION ${HOUDINI_TOOL_INS_DIR}/include/obsidian)
