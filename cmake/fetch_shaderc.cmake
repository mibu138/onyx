function(fetch_shaderc)
        FetchContent_Declare(
            glslang
            GIT_REPOSITORY https://github.com/KhronosGroup/glslang
            GIT_TAG c94ec9356f1a12876ae959409ec67eddb8a90c16)
        set(SKIP_GLSLANG_INSTALL ON)

        FetchContent_Declare(
            spirv_headers
            GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
            GIT_TAG e7b49d7fb59808a650618e0a4008d4bae927e112)
        set(SPIRV_HEADERS_SKIP_EXAMPLES ON)
        set(SPIRV_HEADERS_SKIP_INSTALL ON)

        FetchContent_Declare(
            spirv_tools 
            GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
            GIT_TAG 5737dbb068da91274de9728aab8b4bf27c52f38d)
        set(SPIRV_BUILD_FUZZER OFF)
        set(SKIP_SPIRV_TOOLS_INSTALL ON)
        set(SPIRV_COLOR_TERMINAL OFF)
        set(SPIRV_SKIP_TESTS ON)
        set(SPIRV_SKIP_EXECUTABLES ON)

        FetchContent_Declare(
            shaderc 
            GIT_REPOSITORY https://github.com/google/shaderc
            GIT_TAG fadb0edb247a1daa74f9a206a27e9a1c0417ce49)
        set(SHADERC_SKIP_TESTS ON)
        set(SHADERC_SKIP_EXAMPLES ON)
        set(SHADERC_SKIP_INSTALL ON)
        FetchContent_MakeAvailable(glslang spirv_headers spirv_tools shaderc)
endfunction()

function(make_shaderc_available)
endfunction()
