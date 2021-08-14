list(APPEND SRCS 
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
	common.c       
	locations.c)
list(APPEND DEPS 
    Vulkan::Vulkan)
if(FREETYPE_FOUND)
    list(APPEND SRCS
	ui.c         
	text.c)
    list(APPEND DEPS 
    Freetype::Freetype)
    set(FT2_DIR ${FREETYPE_INCLUDE_DIR_ft2build})
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(obsidianObj OBJECT)
add_library(obsidian    SHARED)
target_sources(obsidianObj PRIVATE ${SRCS})
target_include_directories(obsidianObj 
    PRIVATE "../include/obsidian" ${FT2_DIR}
    INTERFACE "../include")
target_link_libraries(obsidianObj 
    PRIVATE   thirdparty 
    PUBLIC ${DEPS} Hell::HellObj Coal::CoalObj)
target_link_libraries(obsidian PUBLIC Hell::Hell Coal::Coal obsidianObj)
if(WIN32)
set_target_properties(obsidian PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()
add_library(Obsidian::ObsidianObj ALIAS obsidianObj)
add_library(Obsidian::Obsidian ALIAS obsidian)

if(UNIX)
    target_compile_definitions(obsidianObj PUBLIC UNIX)
elseif(WIN32)
    target_compile_definitions(obsidianObj PUBLIC WIN32)
else()
    message(FATAL_ERROR "Need additional platform checks if not on unix.")
endif()
