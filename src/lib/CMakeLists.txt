set(SRCS 
    video.c      
    memory.c     
	image.c      
	command.c    
	loader.c     
    swapchain.c  
    render.c     
    pipeline.c   
	renderpass.c 
	raytrace.c   
	geo.c        
	scene.c      
	file.c       
	ui.c         
	text.c       
	common.c       
	locations.c)

set(PUBLIC_HEADERS
    obsidian.h
    common.h
    dtags.h
    file.h
    filegeo.h
    locations.h
    max_prims.h
    framebuffer.h
    api.h
    attribute.h
    geo.h
    pipeline.h
    raytrace.h
    render.h
    renderpass.h
    scene.h
    text.h
    types.h
    ui.h
    command.h
    def.h
    image.h
    memory.h
    swapchain.h
    video.h
    vulkan.h)

include(author_library)
author_library(obsidian 
    EXPORT_NAME Obsidian
    SOURCES ${SRCS}
    PUBLIC_HEADERS ${PUBLIC_HEADERS}
    DEPS Coal::Coal Hell::Hell Vulkan::Vulkan Freetype::Freetype
    EXTRA_INC_DIRS ${CMAKE_SOURCE_DIR}/thirdparty)

get_target_property(vklib Vulkan::Vulkan IMPORTED_LOCATION)
message("OBDN: Vulkan imported location: ${vklib}")

author_library(obsidianObj 
    TYPE OBJECT
    EXPORT_NAME ObsidianObj
    SOURCES ${SRCS}
    PUBLIC_HEADERS ${PUBLIC_HEADERS}
    DEPS Coal::Coal Hell::Hell Vulkan::Vulkan Freetype::Freetype
    EXTRA_INC_DIRS ${CMAKE_SOURCE_DIR}/thirdparty)
