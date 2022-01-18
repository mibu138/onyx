function(fetch_shaderc)
        FetchContent_Declare(
            shaderc 
            GIT_REPOSITORY https://github.com/mokchira/shaderc
            GIT_TAG main)
        set(SPIRV_HEADERS_SKIP_EXAMPLES ON)
        set(SKIP_GLSLANG_INSTALL ON)
        set(SPIRV_HEADERS_SKIP_INSTALL ON)
        set(SPIRV_BUILD_FUZZER OFF)
        set(SKIP_SPIRV_TOOLS_INSTALL ON)
        set(SPIRV_COLOR_TERMINAL OFF)
        set(SPIRV_SKIP_TESTS ON)
        set(SPIRV_SKIP_EXECUTABLES ON)
        set(SHADERC_SKIP_TESTS ON)
        set(SHADERC_SKIP_EXAMPLES ON)
        set(SHADERC_SKIP_INSTALL ON)
        FetchContent_MakeAvailable(shaderc)
endfunction()

function(make_shaderc_available)
        FetchContent_MakeAvailable(shaderc)
endfunction()
