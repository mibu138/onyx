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
add_library(obsidian STATIC)
target_sources(obsidian PRIVATE ${SRCS})
target_include_directories(obsidian
    PRIVATE "../include/obsidian"
    INTERFACE "../include")
target_link_libraries(obsidian
    PRIVATE   thirdparty 
    PUBLIC ${DEPS} Hell::Hell Coal::Coal)
if(WIN32)
set_target_properties(obsidian PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif()
add_library(Obsidian::Obsidian ALIAS obsidian)

if(UNIX)
    target_compile_definitions(obsidian PUBLIC UNIX)
elseif(WIN32)
    target_compile_definitions(obsidian PUBLIC WIN32)
else()
    message(FATAL_ERROR "Need additional platform checks if not on unix.")
endif()
