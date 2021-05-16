find_package(Freetype REQUIRED FATAL_ERROR)

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
target_include_directories(obsidian PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty ${FREETYPE_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS})
target_link_libraries(obsidian PRIVATE coal Hell::Hell Vulkan::Vulkan Freetype::Freetype)

set_target_properties(obsidian PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
set_target_properties(obsidian PROPERTIES EXPORT_NAME Obsidian)

set(SHADER_TARGET_NAME obsidian-shaders)
add_subdirectory(shaders)
add_dependencies(obsidian ${SHADER_TARGET_NAME})

install(TARGETS obsidian EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/obsidian)

add_library(${PROJECT_NAME}::Obsidian ALIAS obsidian)
