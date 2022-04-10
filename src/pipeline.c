#include "pipeline.h"
#include "raytrace.h"
#include "video.h"
#include "render.h"
#include "vulkan.h"
#include "private.h"
#include "locations.h"
#include "dtags.h"
#include <hell/common.h>
#include <hell/debug.h>
#include <hell/len.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "glslang-wrapper.h"
#include <shaderc/shaderc.h>

enum shaderStageType { VERT, FRAG };

#define SCRATCH_BUFFER_SIZE 2000
int         scratch_marker = 0;
static char scratch_buffer[SCRATCH_BUFFER_SIZE];

static void
reset_scratch_buffer(void)
{
    scratch_marker = 0;
}

static void*
scratch_alloc(int size)
{
    assert(size + scratch_marker < SCRATCH_BUFFER_SIZE);
    int offset = scratch_marker;
    scratch_marker += size;
    return scratch_buffer + offset ;
}

// universal prefix path to search at runtime for shaders.
static char* runtimeSpvPrefix; 

void onyx_SetRuntimeSpvPrefix(const char* prefix)
{
    assert(runtimeSpvPrefix == NULL); // should only be set once
    runtimeSpvPrefix = hell_CopyString(prefix);
}

static void setBlendModeOver(VkPipelineColorBlendAttachmentState* state)
{
    state->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state->colorBlendOp = VK_BLEND_OP_ADD;
    state->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    state->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state->alphaBlendOp = VK_BLEND_OP_ADD;
}

static void setBlendModeOverNoPremul(VkPipelineColorBlendAttachmentState* state)
{
    state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state->colorBlendOp = VK_BLEND_OP_ADD;
    state->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    state->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state->alphaBlendOp = VK_BLEND_OP_ADD;
}

static void setBlendModeErase(VkPipelineColorBlendAttachmentState* state)
{
    state->srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state->colorBlendOp = VK_BLEND_OP_ADD;
    state->srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    state->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state->alphaBlendOp = VK_BLEND_OP_ADD;
}

static void setBlendModeOverMonochrome(VkPipelineColorBlendAttachmentState* state)
{
    state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    state->colorBlendOp = VK_BLEND_OP_ADD;
    state->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    state->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    state->alphaBlendOp = VK_BLEND_OP_ADD;
}

static void setBlendModeOverNoPremulMonochrome(VkPipelineColorBlendAttachmentState* state)
{
    state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    state->colorBlendOp = VK_BLEND_OP_ADD;
    state->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    state->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    state->alphaBlendOp = VK_BLEND_OP_ADD;
}

static void setBlendModeEraseMonochrome(VkPipelineColorBlendAttachmentState* state)
{
    state->srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    state->colorBlendOp = VK_BLEND_OP_ADD;
    state->srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    state->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    state->alphaBlendOp = VK_BLEND_OP_ADD;
}

static void initShaderModule(const VkDevice device, const char* filepath, VkShaderModule* module)
{
    int fr;
    FILE* fp;
    fp = fopen(filepath, "rb");
    if (fp == 0)
    {
        hell_Error(HELL_ERR_FATAL, "Failed to open %s\n", filepath);
    }
    fr = fseek(fp, 0, SEEK_END);
    if (fr != 0)
    {
        hell_Error(HELL_ERR_FATAL, "Seek failed on %s\n", filepath);
    }
    assert( fr == 0 ); // success 
    size_t codeSize = ftell(fp);
    rewind(fp);

    unsigned char* code = hell_Malloc(codeSize);
    fread(code, 1, codeSize, fp);
    fclose(fp);

    const VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode = (uint32_t*)code,
    };

    V_ASSERT( vkCreateShaderModule(device, &shaderInfo, NULL, module) );
    hell_Free(code);
}

#define SPVDIR "onyx"

static void setResolvedShaderPath(const char* shaderName, char* pathBuffer)
{
    const int shaderNameLen = strnlen(shaderName, ONYX_MAX_PATH_LEN);
    if (shaderNameLen >= ONYX_MAX_PATH_LEN)
        hell_Error(HELL_ERR_FATAL, "Shader path length exceeds limit.");
    hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Looking for shader at %s\n", shaderName);
    if (hell_FileExists(shaderName)) // check if shader name is actually a valid path to an spv file
    {
        strcpy(pathBuffer, shaderName);
        return;
    }
    if (runtimeSpvPrefix)
    {
        strcpy(pathBuffer, runtimeSpvPrefix);
        strcat(pathBuffer, shaderName);
        hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Looking for shader at %s\n", pathBuffer);
        if (hell_FileExists(pathBuffer))
            return;
        strcpy(pathBuffer, runtimeSpvPrefix);
        strcat(pathBuffer, "onyx/");
        strcat(pathBuffer, shaderName);
        hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Looking for shader at %s\n", pathBuffer);
        if (hell_FileExists(pathBuffer))
            return;
    }
    const char* localShaderDir = "./shaders/";
    strcpy(pathBuffer, localShaderDir);
    strcat(pathBuffer, shaderName);
    hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Looking for shader at %s\n", pathBuffer);
    if (hell_FileExists(pathBuffer))
        return;
    strcpy(pathBuffer, localShaderDir);
    strcat(pathBuffer, "onyx/");
    strcat(pathBuffer, shaderName);
    hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Looking for shader at %s\n", pathBuffer);
    if (hell_FileExists(pathBuffer))
        return;
    const char* unixShaderDir = "/usr/local/share/shaders/";
    strcpy(pathBuffer, unixShaderDir);
    strcat(pathBuffer, shaderName);
    hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Looking for shader at %s\n", pathBuffer);
    if (hell_FileExists(pathBuffer))
        return;
    strcpy(pathBuffer, unixShaderDir);
    strcat(pathBuffer, "onyx/");
    strcat(pathBuffer, shaderName);
    hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Looking for shader at %s\n", pathBuffer);
    if (hell_FileExists(pathBuffer))
        return;
    const char* onyxRoot = onyx_GetOnyxRoot();
    if (strlen(onyxRoot) + shaderNameLen + strlen(SPVDIR) > ONYX_MAX_PATH_LEN)
        hell_Error(HELL_ERR_FATAL, "Cumulative shader path length exceeds limit.");
    strcpy(pathBuffer, onyxRoot);
    strcat(pathBuffer, SPVDIR);
    strcat(pathBuffer, shaderName);
    hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Looking for shader at %s\n", pathBuffer);
    if (hell_FileExists(pathBuffer))
        return;
    hell_Error(HELL_ERR_FATAL, "Shader %s not found.", shaderName);
}

static const char* getResolvedSprivPath(const char* shaderName)
{
    static char spvpath[ONYX_MAX_PATH_LEN];
    setResolvedShaderPath(shaderName, spvpath);
    hell_DebugPrint(ONYX_DEBUG_TAG_SHADE, "Resolved spv path: %s\n", spvpath);
    return spvpath;
}

#define MAX_GP_SHADER_STAGES 4
#define MAX_ATTACHMENT_STATES 8

void onyx_CreateGraphicsPipelines(const VkDevice device, const uint8_t count, const Onyx_GraphicsPipelineInfo pipelineInfos[/*count*/], 
        VkPipeline pipelines[/*count*/])
{
    // worrisome amount of stack allocations here.. may want to transistion to dynamic allocations for these buffers.
    VkPipelineShaderStageCreateInfo           shaderStages[ONYX_MAX_PIPELINES][MAX_GP_SHADER_STAGES];
    VkPipelineVertexInputStateCreateInfo      vertexInputStates[ONYX_MAX_PIPELINES];
    VkPipelineInputAssemblyStateCreateInfo    inputAssemblyStates[ONYX_MAX_PIPELINES];
    VkPipelineTessellationStateCreateInfo     tessellationStates[ONYX_MAX_PIPELINES];
    VkPipelineViewportStateCreateInfo         viewportStates[ONYX_MAX_PIPELINES];
    VkPipelineRasterizationStateCreateInfo    rasterizationStates[ONYX_MAX_PIPELINES];
    VkPipelineMultisampleStateCreateInfo      multisampleStates[ONYX_MAX_PIPELINES];
    VkPipelineDepthStencilStateCreateInfo     depthStencilStates[ONYX_MAX_PIPELINES];
    VkPipelineColorBlendStateCreateInfo       colorBlendStates[ONYX_MAX_PIPELINES];
    VkPipelineDynamicStateCreateInfo          dynamicStates[ONYX_MAX_PIPELINES];

    VkViewport viewports[ONYX_MAX_PIPELINES];
    VkRect2D   scissors[ONYX_MAX_PIPELINES];

    VkPipelineColorBlendAttachmentState attachmentStates[ONYX_MAX_PIPELINES][MAX_ATTACHMENT_STATES];

    VkGraphicsPipelineCreateInfo createInfos[ONYX_MAX_PIPELINES];

    for (int i = 0; i < count; i++) 
    {
        const Onyx_GraphicsPipelineInfo* rasterInfo = &pipelineInfos[i];
        assert( rasterInfo->vertShader && rasterInfo->fragShader ); // must have at least these 2

        VkShaderModule vertModule;
        VkShaderModule fragModule;
        VkShaderModule tessCtrlModule;
        VkShaderModule tessEvalModule;

        initShaderModule(device, getResolvedSprivPath(rasterInfo->vertShader), &vertModule);
        initShaderModule(device, getResolvedSprivPath(rasterInfo->fragShader), &fragModule);

        uint8_t shaderStageCount = 2;
        for (int j = 0; j < MAX_GP_SHADER_STAGES; j++) 
        {
            shaderStages[i][j] = (VkPipelineShaderStageCreateInfo){0};
        }

        shaderStages[i][0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[i][0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[i][0].module = vertModule;
        shaderStages[i][0].pName = "main";
        shaderStages[i][0].pSpecializationInfo = rasterInfo->pVertSpecializationInfo;
        shaderStages[i][1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[i][1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[i][1].module = fragModule;
        shaderStages[i][1].pName = "main";
        shaderStages[i][1].pSpecializationInfo = rasterInfo->pFragSpecializationInfo;
        if (rasterInfo->tessCtrlShader)
        {
            assert(rasterInfo->tessEvalShader);
            initShaderModule(device, getResolvedSprivPath(rasterInfo->tessCtrlShader), &tessCtrlModule);
            initShaderModule(device, getResolvedSprivPath(rasterInfo->tessEvalShader), &tessEvalModule);
            shaderStages[i][2].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i][2].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            shaderStages[i][2].module = tessCtrlModule;
            shaderStages[i][2].pName = "main";
            shaderStages[i][2].pSpecializationInfo = NULL;
            shaderStages[i][3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i][3].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            shaderStages[i][3].module = tessEvalModule;
            shaderStages[i][3].pName = "main";
            shaderStages[i][3].pSpecializationInfo = NULL;
            shaderStageCount += 2;
        }

        if (rasterInfo->vertexDescription.attributeCount == 0)
                hell_DebugPrint(ONYX_DEBUG_TAG_GRAPHIC_PIPE, "rasterInfo.vertexDescription.attributeCount == 0. Assuming verts defined in shader.\n");

        vertexInputStates[i] = (VkPipelineVertexInputStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount =   rasterInfo->vertexDescription.bindingCount,
            .pVertexBindingDescriptions =      rasterInfo->vertexDescription.bindingDescriptions,
            .vertexAttributeDescriptionCount = rasterInfo->vertexDescription.attributeCount,
            .pVertexAttributeDescriptions =    rasterInfo->vertexDescription.attributeDescriptions
        };

        inputAssemblyStates[i] = (VkPipelineInputAssemblyStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = rasterInfo->primitiveTopology,
            .primitiveRestartEnable = VK_FALSE // applies only to index calls
        };

        VkExtent2D viewportDim = rasterInfo->viewportDim;
        if (viewportDim.width < 1) // use window size
        {
            bool hasDynamicViewportEnabled = false;
            bool hasDynamicScissorEnabled  = false;
            for (int j = 0; j < rasterInfo->dynamicStateCount; j++)
            {
                if (rasterInfo->pDynamicStates[j] == VK_DYNAMIC_STATE_VIEWPORT)
                    hasDynamicViewportEnabled = true;
                if (rasterInfo->pDynamicStates[j] == VK_DYNAMIC_STATE_SCISSOR)
                    hasDynamicScissorEnabled = true;
            }
            assert(hasDynamicViewportEnabled);
            assert(hasDynamicScissorEnabled);
            // no longer allow this not to rely on ONYX_WINDOW_* .
            // if we don't specify a viewportDim parameter then we must
            // enable dynamic viewport state
        }

        viewports[i] = (VkViewport){
            .height = viewportDim.height,
            .width = viewportDim.width,
            .x = 0,
            .y = 0,
            .minDepth = 0.0,
            .maxDepth = 1.0
        };

        scissors[i] = (VkRect2D){
            .extent = {viewportDim.width, viewportDim.height},
            .offset = {0, 0}
        };

        viewportStates[i] = (VkPipelineViewportStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .scissorCount = 1,
            .pScissors = &scissors[i],
            .viewportCount = 1,
            .pViewports = &viewports[i],
        };

        rasterizationStates[i] = (VkPipelineRasterizationStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE, // dunno
            .rasterizerDiscardEnable = VK_FALSE, // actually discards everything
            .polygonMode = rasterInfo->polygonMode,
            // this line won't work because 0 == VK_CULL_MODE_NONE
            //.cullMode = rasterInfo->cullMode == 0 ? VK_CULL_MODE_BACK_BIT : rasterInfo->cullMode,
            .cullMode = rasterInfo->cullMode,
            .frontFace = rasterInfo->frontFace,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0
        };

        multisampleStates[i] = (VkPipelineMultisampleStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable = VK_FALSE,
            .rasterizationSamples = rasterInfo->sampleCount 
            // TODO: alot more settings here. more to look into
        };

        const uint32_t attachmentCount = rasterInfo->attachmentCount > 0 ? rasterInfo->attachmentCount : 1;
        assert(attachmentCount < MAX_ATTACHMENT_STATES);
        for (int j = 0; j < attachmentCount; j++) 
        {
            attachmentStates[i][j] = (VkPipelineColorBlendAttachmentState){
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, /* need this to actually
                                                                            write anything to the
                                                                            framebuffer */
                .blendEnable = (rasterInfo->blendMode == ONYX_BLEND_MODE_NONE) ? VK_FALSE : VK_TRUE, 
            };
        }

        switch (rasterInfo->blendMode)
        {
            case ONYX_BLEND_MODE_OVER:                      setBlendModeOver(&attachmentStates[i][0]); break;
            case ONYX_BLEND_MODE_OVER_NO_PREMUL:            setBlendModeOverNoPremul(&attachmentStates[i][0]); break;
            case ONYX_BLEND_MODE_ERASE:                     setBlendModeErase(&attachmentStates[i][0]); break;
            case ONYX_BLEND_MODE_OVER_MONOCHROME:           setBlendModeOverMonochrome(&attachmentStates[i][0]); break;
            case ONYX_BLEND_MODE_OVER_NO_PREMUL_MONOCHROME: setBlendModeOverNoPremulMonochrome(&attachmentStates[i][0]); break;
            case ONYX_BLEND_MODE_ERASE_MONOCHROME:          setBlendModeEraseMonochrome(&attachmentStates[i][0]); break;
            case ONYX_BLEND_MODE_NONE: break;
        }

        colorBlendStates[i] = (VkPipelineColorBlendStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE, // only for integer framebuffer formats
            .logicOp = 0,
            .attachmentCount = attachmentCount,
            .pAttachments = attachmentStates[i] /* must have independentBlending device   
                feature enabled for these to be different. each entry would correspond 
                to the blending for a different framebuffer. */
        };

        depthStencilStates[i] = (VkPipelineDepthStencilStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
            .depthBoundsTestEnable = VK_FALSE, // allows you to only keep fragments within the depth bounds
            .stencilTestEnable = VK_FALSE,
        };

        tessellationStates[i] = (VkPipelineTessellationStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
            .patchControlPoints = rasterInfo->tesselationPatchPoints,
        };

        assert(shaderStageCount <= MAX_GP_SHADER_STAGES);
        assert(rasterInfo->renderPass != VK_NULL_HANDLE);
        assert(rasterInfo->layout != VK_NULL_HANDLE);

        dynamicStates[i] = (VkPipelineDynamicStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = rasterInfo->dynamicStateCount,
            .pDynamicStates = rasterInfo->pDynamicStates
        };

        createInfos[i] = (VkGraphicsPipelineCreateInfo){
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .basePipelineIndex = 0, // not used
            .basePipelineHandle = 0,
            .subpass = rasterInfo->subpass, // which subpass in the renderpass do we use this pipeline with
            .renderPass = rasterInfo->renderPass,
            .layout = rasterInfo->layout,
            .pDynamicState = &dynamicStates[i],
            .pColorBlendState = &colorBlendStates[i],
            .pDepthStencilState = &depthStencilStates[i],
            .pMultisampleState = &multisampleStates[i],
            .pRasterizationState = &rasterizationStates[i],
            .pViewportState = &viewportStates[i],
            .pTessellationState = &tessellationStates[i], // may be able to do splines with this
            .flags = 0,
            .stageCount = shaderStageCount,
            .pStages = shaderStages[i],
            .pVertexInputState = &vertexInputStates[i],
            .pInputAssemblyState = &inputAssemblyStates[i],
        };
    }

    V_ASSERT( vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, count, createInfos, 
                NULL, pipelines) );

    for (int i = 0; i < count; i++) 
    {
        for (int j = 0; j < MAX_GP_SHADER_STAGES; j++) 
        {
            if (shaderStages[i][j].module != VK_NULL_HANDLE)
                vkDestroyShaderModule(device, shaderStages[i][j].module, NULL);
        }
    }
}

#define MAX_RT_SHADER_COUNT 10

void onyx_CreateRayTracePipelines(VkDevice device, Onyx_Memory* memory, const uint8_t count, const Onyx_RayTracePipelineInfo pipelineInfos[/*count*/], 
        VkPipeline pipelines[/*count*/], Onyx_ShaderBindingTable shaderBindingTables[/*count*/])
{
    assert(count > 0);
    assert(count < ONYX_MAX_PIPELINES);

    VkRayTracingPipelineCreateInfoKHR createInfos[ONYX_MAX_PIPELINES];

    VkPipelineShaderStageCreateInfo               shaderStages[ONYX_MAX_PIPELINES][MAX_RT_SHADER_COUNT];
    VkRayTracingShaderGroupCreateInfoKHR          shaderGroups[ONYX_MAX_PIPELINES][MAX_RT_SHADER_COUNT];
    VkPipelineLibraryCreateInfoKHR                libraryInfos[ONYX_MAX_PIPELINES];
    //VkRayTracingPipelineInterfaceCreateInfoKHR    libraryInterfaces[count];
    //VkPipelineDynamicStateCreateInfo              dynamicStates[count];
    
    memset(shaderStages, 0, sizeof(shaderStages));
    memset(shaderGroups, 0, sizeof(shaderGroups));
    memset(libraryInfos, 0, sizeof(libraryInfos));
    memset(createInfos,  0, sizeof(createInfos));

    for (int p = 0; p < count; p++) 
    {
        const Onyx_RayTracePipelineInfo* rayTraceInfo = &pipelineInfos[p];

        assert(rayTraceInfo->layout != VK_NULL_HANDLE);

        const uint8_t raygenCount = rayTraceInfo->raygenCount;
        const uint8_t missCount   = rayTraceInfo->missCount;
        const uint8_t chitCount   = rayTraceInfo->chitCount;

        assert( raygenCount == 1 ); // for now
        assert( missCount < 10 );
        assert( chitCount < 10 ); // make sure its in a reasonable range

        VkShaderModule raygenSM[MAX_RT_SHADER_COUNT];
        VkShaderModule missSM[MAX_RT_SHADER_COUNT];
        VkShaderModule chitSM[MAX_RT_SHADER_COUNT];

        memset(raygenSM, 0, sizeof(raygenSM));
        memset(missSM, 0, sizeof(missSM));
        memset(chitSM, 0, sizeof(chitSM));

        char* const* const raygenShaders = rayTraceInfo->raygenShaders;
        char* const* const missShaders =   rayTraceInfo->missShaders;
        char* const* const chitShaders =   rayTraceInfo->chitShaders;

        for (int i = 0; i < raygenCount; i++) 
            initShaderModule(device, getResolvedSprivPath(raygenShaders[i]), &raygenSM[i]);
        for (int i = 0; i < missCount; i++) 
            initShaderModule(device, getResolvedSprivPath(missShaders[i]), &missSM[i]);
        for (int i = 0; i < chitCount; i++) 
            initShaderModule(device, getResolvedSprivPath(chitShaders[i]), &chitSM[i]);

        const int shaderCount = raygenCount + missCount + chitCount;

        assert(shaderCount < MAX_RT_SHADER_COUNT);

        for (int i = 0; i < shaderCount; i++) 
        {
            shaderStages[p][i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[p][i].pName = "main";

            shaderGroups[p][i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroups[p][i].pShaderGroupCaptureReplayHandle = NULL;
            shaderGroups[p][i].anyHitShader       = VK_SHADER_UNUSED_KHR;
            shaderGroups[p][i].closestHitShader   = VK_SHADER_UNUSED_KHR;
            shaderGroups[p][i].generalShader      = VK_SHADER_UNUSED_KHR;
            shaderGroups[p][i].intersectionShader = VK_SHADER_UNUSED_KHR;

            if (i < raygenCount)
            {
                int m = i - 0;
                shaderStages[p][i].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                shaderStages[p][i].module = raygenSM[m];

                shaderGroups[p][i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                shaderGroups[p][i].generalShader = i;

            }
            else if ( i - raygenCount < missCount )
            {
                int m = i - raygenCount;
                shaderStages[p][i].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
                shaderStages[p][i].module = missSM[m];

                shaderGroups[p][i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                shaderGroups[p][i].generalShader = i;
            }
            else if ( (i - raygenCount) - missCount < chitCount )
            {
                int m = (i - raygenCount) - missCount;
                shaderStages[p][i].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                shaderStages[p][i].module = chitSM[m];

                shaderGroups[p][i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                shaderGroups[p][i].closestHitShader = i;
            }
        }

        libraryInfos[p] = (VkPipelineLibraryCreateInfoKHR){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
        };

        assert(rayTraceInfo->layout != VK_NULL_HANDLE);

        createInfos[p] = (VkRayTracingPipelineCreateInfoKHR){
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
            .maxPipelineRayRecursionDepth = 1,
            .layout     = rayTraceInfo->layout,
            .pLibraryInfo = &libraryInfos[p],
            .groupCount = shaderCount,
            .stageCount = shaderCount,
            .pGroups    = shaderGroups[p],
            .pStages    = shaderStages[p]
        };
    }

    V_ASSERT( vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, count, createInfos, NULL, pipelines) );

    for (int i = 0; i < count; i++) 
    {
        onyx_CreateShaderBindingTable(memory, createInfos[i].groupCount, pipelines[i], &shaderBindingTables[i]);
    }

    for (int i = 0; i < count; i++) 
    {
        const int sc = createInfos[i].stageCount;
        for (int j = 0; j < sc; j++) 
        {
            vkDestroyShaderModule(device, createInfos[i].pStages[j].module, NULL);
        }
    }
}

void onyx_CreateDescriptorSetLayouts(VkDevice device, const uint8_t count, const Onyx_DescriptorSetInfo sets[/*count*/],
        VkDescriptorSetLayout layouts[/*count*/])
{
    // counters for different descriptors
    assert( count < ONYX_MAX_DESCRIPTOR_SETS);
    for (int i = 0; i < count; i++) 
    {
        const Onyx_DescriptorSetInfo set = sets[i];
        assert(set.bindingCount > 0);
        assert(set.bindingCount <= ONYX_MAX_BINDINGS);
        VkDescriptorBindingFlags     bindFlags[ONYX_MAX_BINDINGS];
        VkDescriptorSetLayoutBinding bindings[ONYX_MAX_BINDINGS];
        for (int b = 0; b < set.bindingCount; b++) 
        {
            const uint32_t dCount = set.bindings[b].descriptorCount;

            bindings[b].binding = b;
            bindings[b].descriptorCount = dCount;
            bindings[b].descriptorType  = set.bindings[b].type;
            bindings[b].stageFlags      = set.bindings[b].stageFlags;
            bindings[b].pImmutableSamplers = NULL;

            bindFlags[b] = set.bindings[b].bindingFlags;
        }

        // this is only useful for texture arrays really. not sure what the performance implications
        // are for enabling it for all descriptor sets. may want to make it an optional parameter.

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = set.bindingCount,
            .pBindingFlags = bindFlags
        };


        const VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &flagsInfo,
            .bindingCount = set.bindingCount,
            .pBindings = bindings
        };
        V_ASSERT(vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &layouts[i]));
    }
}

void onyx_CreateDescriptorSets(VkDevice device, const uint8_t count, const Onyx_DescriptorSetInfo sets[/*count*/], 
        const VkDescriptorSetLayout layouts[/*count*/],
        Onyx_R_Description* out)
{
    // counters for different descriptors
    assert( count < ONYX_MAX_DESCRIPTOR_SETS);

    out->descriptorSetCount = count;

    int dcUbo = 0, dcAs = 0, dcSi = 0, dcSb = 0, dcCis = 0, dcIa = 0;
    for (int i = 0; i < count; i++) 
    {
        const Onyx_DescriptorSetInfo set = sets[i];
        assert(set.bindingCount > 0);
        assert(set.bindingCount <= ONYX_MAX_BINDINGS);
        assert(out->descriptorSets[i] == VK_NULL_HANDLE);
        for (int b = 0; b < set.bindingCount; b++) 
        {
            const uint32_t dCount = set.bindings[b].descriptorCount;

            switch (set.bindings[b].type) 
            {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             dcUbo += dCount; break;
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: dcAs  += dCount; break;
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:              dcSi  += dCount; break;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:             dcSb  += dCount; break;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:     dcCis += dCount; break;
                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:           dcIa  += dCount; break;
                default: assert(false);
            }
        }
    }

    // were not allowed to specify a count of 0
    dcUbo = dcUbo > 0 ? dcUbo : 1;
    dcAs  = dcAs > 0  ? dcAs  : 1;
    dcSi  = dcSi > 0  ? dcSi  : 1;
    dcSb  = dcSb > 0  ? dcSb  : 1;
    dcCis = dcCis > 0 ? dcCis : 1;
    dcIa  = dcIa   > 0 ? dcIa : 1;

    const VkDescriptorPoolSize poolSizes[] = {{
            .descriptorCount = dcUbo,
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        },{
            .descriptorCount = dcAs,
            .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        },{
            .descriptorCount = dcSi,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        },{
            .descriptorCount = dcSb,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        },{
            .descriptorCount = dcCis,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        },{
            .descriptorCount = dcIa,
            .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
    }};

    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = count,
        .poolSizeCount = LEN(poolSizes),
        .pPoolSizes = poolSizes, 
    };

    V_ASSERT(vkCreateDescriptorPool(device, &poolInfo, NULL, &out->descriptorPool));

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = out->descriptorPool,
        .descriptorSetCount = out->descriptorSetCount,
        .pSetLayouts = layouts
    };

    V_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, out->descriptorSets));
}

void onyx_CreatePipelineLayouts(VkDevice device, const uint8_t count, const Onyx_PipelineLayoutInfo layoutInfos[/*static count*/], 
        VkPipelineLayout pipelineLayouts[/*count*/])
{
    assert(count < ONYX_MAX_PIPELINES);
    for (int i = 0; i < count; i++) 
    {
        const Onyx_PipelineLayoutInfo* layoutInfo = &layoutInfos[i];
        assert(layoutInfo->pushConstantCount  < ONYX_MAX_PUSH_CONSTANTS);
        assert(layoutInfo->descriptorSetCount < ONYX_MAX_DESCRIPTOR_SETS);

        //assert(dsCount > 0 && dsCount < ONYX_MAX_DESCRIPTOR_SETS);

        VkPipelineLayoutCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .setLayoutCount = layoutInfo->descriptorSetCount,
            .pSetLayouts = layoutInfo->descriptorSetLayouts,
            .pushConstantRangeCount = layoutInfo->pushConstantCount,
            .pPushConstantRanges = layoutInfo->pushConstantsRanges
        };

        V_ASSERT(vkCreatePipelineLayout(device, &info, NULL, &pipelineLayouts[i]));
    }
}

void onyx_create_pipeline_layout(VkDevice device, const Onyx_PipelineLayoutInfo* li,
        VkPipelineLayout* layout)
{
    VkPipelineLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = li->descriptorSetCount,
        .pSetLayouts = li->descriptorSetLayouts,
        .pushConstantRangeCount = li->pushConstantCount,
        .pPushConstantRanges = li->pushConstantsRanges
    };

    V_ASSERT(vkCreatePipelineLayout(device, &info, NULL, layout));
}

void onyx_r_CleanUpPipelines()
{
    hell_DebugPrint(ONYX_DEBUG_TAG_PIPE, "called. no longer does anything\n");
}

void onyx_DestroyDescription(const VkDevice device, Onyx_R_Description* d)
{
    vkDestroyDescriptorPool(device, d->descriptorPool, NULL);
    memset(d, 0, sizeof(*d));
}

void onyx_CreateDescriptionsAndLayouts(VkDevice device, const uint32_t descSetCount, const Onyx_DescriptorSetInfo sets[/*descSetCount*/], 
        VkDescriptorSetLayout layouts[/*descSetCount*/], 
        const uint32_t descriptionCount, Onyx_R_Description descriptions[/*descSetCount*/])
{
    onyx_CreateDescriptorSetLayouts(device, descSetCount, sets, layouts);
    for (int i = 0; i < descriptionCount; i++)
    {
        onyx_CreateDescriptorSets(device, descSetCount, sets, layouts, &descriptions[i]);
    }
}

void onyx_CreateGraphicsPipeline_Taurus(VkDevice device, const VkRenderPass renderPass, const VkPipelineLayout layout, const VkPolygonMode mode, VkPipeline* pipeline)
{
    uint8_t attrSize = 3 * sizeof(float);
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
    Onyx_GraphicsPipelineInfo gpi = {
        .renderPass = renderPass,
        .layout = layout,
        .attachmentCount = 1,
        .sampleCount = VK_SAMPLE_COUNT_1_BIT,
        .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .polygonMode = mode,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
        .vertexDescription = onyx_GetVertexDescription(1, &attrSize),
        .vertShader = "onyx/simple.vert.spv",
        .fragShader = "onyx/simple.frag.spv"
    };
    onyx_CreateGraphicsPipelines(device, 1, &gpi, pipeline);
}

#define MAX_BINDING_COUNT 8 //arbitrary... we should check if there is a device limit on this

void
onyx_CreateDescriptorSetLayout(
    VkDevice device,
    const uint8_t                  bindingCount,
    const Onyx_DescriptorBinding   bindings[/*bindingCount*/],
    VkDescriptorSetLayout*         layout)
{
    assert(bindingCount <= MAX_BINDING_COUNT);
    VkDescriptorBindingFlags     vkbindFlags[MAX_BINDING_COUNT];
    VkDescriptorSetLayoutBinding vkbindings[MAX_BINDING_COUNT];
    for (int b = 0; b < bindingCount; b++)
    {
        vkbindings[b].binding            = b;
        vkbindings[b].descriptorCount    = bindings[b].descriptorCount;
        vkbindings[b].descriptorType     = bindings[b].type;
        vkbindings[b].stageFlags         = bindings[b].stageFlags;
        vkbindings[b].pImmutableSamplers = NULL;
        vkbindFlags[b]                   = bindings[b].bindingFlags;
    }

    // this is only useful for texture arrays really. not sure what the
    // performance implications are for enabling it for all descriptor sets. may
    // want to make it an optional parameter.

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount  = bindingCount,
        .pBindingFlags = vkbindFlags};

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = &flagsInfo,
        .bindingCount = bindingCount,
        .pBindings    = vkbindings};

    V_ASSERT(vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, layout));
}

#define MAX_POOL_SIZES 8

void
onyx_CreateDescriptorPool(VkDevice device,
                            uint32_t uniformBufferCount,
                            uint32_t dynamicUniformBufferCount,
                          uint32_t combinedImageSamplerCount,
                          uint32_t storageImageCount,
                          uint32_t storageBufferCount,
                          uint32_t inputAttachmentCount,
                          uint32_t accelerationStructureCount,
                          VkDescriptorPool* pool)
{
    // if we pass in a pool size at all, desc count cannot be 0 for it, so we must do some funstuff
    VkDescriptorPoolSize uniformBufferPS = {
        .descriptorCount = uniformBufferCount,
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    };

    VkDescriptorPoolSize dynamicUniformBufferPS = {
        .descriptorCount = dynamicUniformBufferCount,
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
    };

    VkDescriptorPoolSize combinedImageSamplerPS = {
        .descriptorCount = combinedImageSamplerCount,
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    };

    VkDescriptorPoolSize storageImagePS = {
        .descriptorCount = storageImageCount,
        .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    };

    VkDescriptorPoolSize storageBufferPS = {
        .descriptorCount = storageBufferCount,
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    };

    VkDescriptorPoolSize inputAttachmentPS = {
        .descriptorCount = inputAttachmentCount,
        .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
    };

    VkDescriptorPoolSize accelerationStructurePS = {
        .descriptorCount = accelerationStructureCount,
        .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
    };

    VkDescriptorPoolSize poolSizes[8];
    memset(poolSizes, 0, sizeof(poolSizes));

    VkDescriptorPoolSize* psp = poolSizes;

    if (uniformBufferCount > 0) *psp++ = uniformBufferPS;
    if (dynamicUniformBufferCount > 0) *psp++ = dynamicUniformBufferPS;
    if (combinedImageSamplerCount > 0) *psp++ = combinedImageSamplerPS;
    if (storageImageCount > 0) *psp++ = storageImagePS;
    if (storageBufferCount > 0) *psp++ = storageBufferPS;
    if (inputAttachmentCount > 0) *psp++ = inputAttachmentPS;
    if (accelerationStructureCount > 0) *psp++ = accelerationStructurePS;

    uint32_t psCount = psp - poolSizes;

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = ONYX_MAX_DESCRIPTOR_SETS,
        .poolSizeCount = psCount,
        .pPoolSizes = poolSizes
    };

    V_ASSERT(vkCreateDescriptorPool(device, &poolInfo, NULL, pool));
}

void
onyx_AllocateDescriptorSets(const VkDevice device, VkDescriptorPool pool, uint32_t descSetCount,
                            const VkDescriptorSetLayout layouts[/*descSetCount*/],
                            VkDescriptorSet*      sets)
{
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = descSetCount,
        .pSetLayouts = layouts
    };

    V_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, sets));
}

// caller must free bytes buffer
static bool
compile_spirv_shaderc(const SpirvCompileInfo* c, u8** bytes_buffer, int* byte_count)
{
#ifdef ONYX_SHADERC_ENABLED
    shaderc_shader_kind shader_kind = 0;
    switch (c->shader_type)
    {
    case ONYX_SHADER_TYPE_VERTEX:   shader_kind = shaderc_vertex_shader; break;
    case ONYX_SHADER_TYPE_FRAGMENT: shader_kind = shaderc_fragment_shader; break;
    case ONYX_SHADER_TYPE_COMPUTE: shader_kind = shaderc_compute_shader; break; 
    case ONYX_SHADER_TYPE_GEOMETRY: 
    case ONYX_SHADER_TYPE_TESS_CONTROL: 
    case ONYX_SHADER_TYPE_TESS_EVALUATION: 
    case ONYX_SHADER_TYPE_RAY_GEN: 
    case ONYX_SHADER_TYPE_ANY_HIT: 
    case ONYX_SHADER_TYPE_CLOSEST_HIT: 
    case ONYX_SHADER_TYPE_MISS: 
    default: hell_Error(HELL_ERR_FATAL, "Onyx: shader type not supported\n");
    }
    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_compilation_result_t result = shaderc_compile_into_spv(
            compiler, c->shader_string, strlen(c->shader_string), shader_kind, 
            c->name, c->entry_point, NULL);
    shaderc_compilation_status comp_status = shaderc_result_get_compilation_status(result);
    size_t num_warnings = shaderc_result_get_num_warnings(result);
    size_t num_errors   = shaderc_result_get_num_errors(result);
    if (num_warnings || num_errors)
    {
        hell_Print("Shader %s Compilation error: %s\n", c->name, 
                shaderc_result_get_error_message(result));
    }
    if (comp_status != shaderc_compilation_status_success)
        hell_Error(HELL_ERR_FATAL, "ERROR: shader compilation failed\n");
    const char* bytes = shaderc_result_get_bytes(result);
    size_t num_bytes  = shaderc_result_get_length(result);
    *byte_count = num_bytes;
    *bytes_buffer = hell_Malloc(num_bytes);
    memcpy(*bytes_buffer, bytes, num_bytes);
    shaderc_result_release(result);
    return true;
#else
    return false;
#endif
}

static bool
compile_spirv_glslang(const SpirvCompileInfo* c, u8** bytes_buffer, int* byte_count)
{
#if ONYX_GLSLANG_ENABLED
    return glsl_to_spirv(c, bytes_buffer, byte_count);
#else
    return false;
#endif
}

void 
onyx_create_shader_module(VkDevice device, const SpirvCompileInfo* sci,
                             VkShaderModule* module)
{
    u8* bytes;
    int byte_count;
#ifdef ONYX_SHADERC_ENABLED
    if (!compile_spirv_shaderc(sci, &bytes, &byte_count))
        assert(0 && "Shader compilation failed");
#elif ONYX_GLSLANG_ENABLED
    if (!compile_spirv_glslang(sci, &bytes, &byte_count))
        assert(0 && "Shader compilation failed");
#else
    assert(0 && "No shader compiler was enabled");
#endif
    VkShaderModuleCreateInfo ci = onyx_ShaderModuleCreateInfo(byte_count, bytes);
    V_ASSERT(vkCreateShaderModule(device, &ci, NULL, module));
    hell_Free(bytes);
}

void 
onyx_CreateShaderModule(VkDevice device, const char* shader_string,
                             const char* name, Onyx_ShaderType type,
                             VkShaderModule* module)
{
    SpirvCompileInfo sci = {
        .entry_point = "main",
        .name = name,
        .shader_type = type,
        .shader_string = shader_string
    };

    onyx_create_shader_module(device, &sci, module);
}

void
onyx_create_graphics_pipeline(VkDevice device, const OnyxGraphicsPipelineInfo* s,
                              const VkPipelineCache cache, VkPipeline* pipeline)
{
    reset_scratch_buffer();

    VkPipelineShaderStageCreateInfo* shader_stages =
        scratch_alloc(sizeof(*shader_stages) * s->shader_stage_count);

    for (int i = 0; i < s->shader_stage_count; i++)
    {
        shader_stages[i] = (VkPipelineShaderStageCreateInfo){
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = s->shader_stages[i].module,
            .pName  = s->shader_stages[i].entry_point,
            .pSpecializationInfo = s->shader_stages[i].specialization_info,
            .stage               = s->shader_stages[i].stage};
    }

    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexAttributeDescriptionCount =
            s->vertex_attribute_description_count,
        .pVertexAttributeDescriptions  = s->vertex_attribute_descriptions,
        .vertexBindingDescriptionCount = s->vertex_binding_description_count,
        .pVertexBindingDescriptions    = s->vertex_binding_descriptions};

    VkPipelineInputAssemblyStateCreateInfo input_assem = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .primitiveRestartEnable = s->primitive_restart_enable,
        .topology               = s->topology};

    VkPipelineTessellationStateCreateInfo tess = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = s->patch_control_points,
    };

    VkViewport viewport = {.width    = s->viewport_width,
                           .height   = s->viewport_height,
                           .x        = 0,
                           .y        = 0,
                           .minDepth = 0.0,
                           .maxDepth = 1.0};

    VkRect2D scissor = {.extent.width  = s->viewport_width,
                        .extent.height = s->viewport_height,
                        .offset.x      = 0,
                        .offset.y      = 0};

    VkPipelineViewportStateCreateInfo view = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .scissorCount  = 1,
        .pScissors     = &scissor,
        .viewportCount = 1,
        .pViewports    = &viewport,
    };

    VkPipelineRasterizationStateCreateInfo raster = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = s->depth_clamp_enable,
        .rasterizerDiscardEnable = s->rasterizer_discard_enable,
        .polygonMode             = s->polygon_mode,
        .cullMode                = s->cull_mode,
        .frontFace               = s->front_face,
        .depthBiasEnable         = s->depth_bias_enable,
        .depthBiasConstantFactor = s->depth_bias_constant_factor,
        .depthBiasClamp          = s->depth_bias_clamp,
        .depthBiasSlopeFactor    = s->depth_bias_slope_factor,
        .lineWidth               = s->line_width,
    };

    VkPipelineMultisampleStateCreateInfo multisamp = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples  = s->rasterization_samples
                                     ? s->rasterization_samples
                                     : VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = s->sample_shading_enable,
        .minSampleShading      = s->min_sample_shading,
        .pSampleMask           = s->p_sample_mask,
        .alphaToCoverageEnable = s->alpha_to_coverage_enable,
        .alphaToOneEnable      = s->alpha_to_one_enable,
    };

    VkPipelineDepthStencilStateCreateInfo depthstencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = s->depth_test_enable,
        .depthWriteEnable      = s->depth_write_enable,
        .depthCompareOp        = (VkCompareOp)(s->depth_compare_op + 1),
        .depthBoundsTestEnable = s->depth_bounds_test_enable,
        .stencilTestEnable     = s->stencil_test_enable,
        .front                 = s->front,
        .back                  = s->back,
        .minDepthBounds        = s->min_depth_bounds,
        .maxDepthBounds        = s->max_depth_bounds,
    };

    VkColorComponentFlags write_all =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendAttachmentState* attachment_blends =
        scratch_alloc(sizeof(attachment_blends[0]) * s->attachment_count);

    for (int i = 0; i < s->attachment_count; i++)
    {
        VkPipelineColorBlendAttachmentState blend_attachment =
            onyx_PipelineColorBlendAttachmentState(
                s->attachment_blends[i].blend_enable, VK_BLEND_FACTOR_SRC_ALPHA,
                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VK_BLEND_OP_ADD, write_all);

        switch (s->attachment_blends[i].blend_mode)
        {
        case ONYX_BLEND_MODE_OVER:
            setBlendModeOver(&blend_attachment);
            break;
        case ONYX_BLEND_MODE_OVER_NO_PREMUL:
            setBlendModeOverNoPremul(&blend_attachment);
            break;
        case ONYX_BLEND_MODE_ERASE:
            setBlendModeErase(&blend_attachment);
            break;
        case ONYX_BLEND_MODE_OVER_MONOCHROME:
            setBlendModeOverMonochrome(&blend_attachment);
            break;
        case ONYX_BLEND_MODE_OVER_NO_PREMUL_MONOCHROME:
            setBlendModeOverNoPremulMonochrome(&blend_attachment);
            break;
        case ONYX_BLEND_MODE_ERASE_MONOCHROME:
            setBlendModeEraseMonochrome(&blend_attachment);
            break;
        case ONYX_BLEND_MODE_NONE:
            break;
        }

        attachment_blends[i] = blend_attachment;
    }

    VkPipelineColorBlendStateCreateInfo colorblend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = s->attachment_count,
        .pAttachments    = attachment_blends,
        .logicOp         = s->logic_op,
        .logicOpEnable   = s->logic_op_enable,
        .blendConstants  = {s->blend_constants[0], s->blend_constants[1],
                           s->blend_constants[2], s->blend_constants[3]}};

    VkPipelineDynamicStateCreateInfo dynstate = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = s->dynamic_state_count,
        .pDynamicStates    = s->dynamic_states};

    VkGraphicsPipelineCreateInfo gpci = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = s->shader_stage_count,
        .pStages             = shader_stages,
        .pVertexInputState   = &vertex_input,
        .pInputAssemblyState = &input_assem,
        .pTessellationState  = &tess,
        .pViewportState      = &view,
        .pRasterizationState = &raster,
        .pMultisampleState   = &multisamp,
        .pDepthStencilState  = &depthstencil,
        .pColorBlendState    = &colorblend,
        .pDynamicState       = &dynstate,
        .layout              = s->layout,
        .renderPass          = s->render_pass,
        .subpass             = s->subpass,
        .basePipelineHandle  = s->base_pipeline,
        .basePipelineIndex   = s->base_pipeline_index,
    };

    V_ASSERT(
        vkCreateGraphicsPipelines(device, cache, 1, &gpci, NULL, pipeline));
}

void 
onyx_CreateGraphicsPipeline_Basic(VkDevice device, VkPipelineLayout layout,
        VkRenderPass renderPass, uint32_t subpass, 
        uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages,
        const VkPipelineVertexInputStateCreateInfo* pVertexInputState,
        VkPrimitiveTopology topology, uint32_t patchControlPoints, uint32_t viewport_width,
        uint32_t viewport_height, bool dynamicViewport,
        VkPolygonMode polygonMode, VkFrontFace frontFace, float lineWidth, bool depthTestEnable, 
        bool depthWriteEnable,
        Onyx_BlendMode blendMode,
        VkPipeline* pipeline)
{
    VkPipelineInputAssemblyStateCreateInfo input_assem = onyx_PipelineInputAssemblyStateCreateInfo(topology, false);
    VkPipelineTessellationStateCreateInfo tess = onyx_PipelineTessellationStateCreateInfo(patchControlPoints);
    VkViewport viewport = {
        .width  = viewport_width,
        .height = viewport_height,
        .x = 0,
        .y = 0,
        .minDepth = 0.0,
        .maxDepth = 1.0
    };
    VkRect2D scissor = {
        .extent.width = viewport_width,
        .extent.height = viewport_height,
        .offset.x = 0,
        .offset.y = 0
    };
    VkPipelineViewportStateCreateInfo view = onyx_PipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);
    VkPipelineRasterizationStateCreateInfo raster = onyx_PipelineRasterizationStateCreateInfo(false,
            false, polygonMode, VK_CULL_MODE_BACK_BIT, frontFace, false, 0.0, 0.0, 0.0, lineWidth);
    VkPipelineMultisampleStateCreateInfo multisamp = onyx_PipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT,
            false, 0.0, NULL, false, false);
    VkStencilOpState stencilopstate = {};
    VkPipelineDepthStencilStateCreateInfo depthstencil = onyx_PipelineDepthStencilStateCreateInfo(depthTestEnable, 
            depthWriteEnable, VK_COMPARE_OP_LESS_OR_EQUAL, false, false, stencilopstate, stencilopstate, 0.0, 0.0);
    bool do_blend = blendMode == ONYX_BLEND_MODE_NONE ? false : true;
    VkColorComponentFlags write_all = 
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendAttachmentState blend_attachment = onyx_PipelineColorBlendAttachmentState(do_blend, VK_BLEND_FACTOR_SRC_ALPHA,
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
            write_all);
    switch (blendMode) 
    {
        case ONYX_BLEND_MODE_OVER:                      setBlendModeOver(&blend_attachment); break;
        case ONYX_BLEND_MODE_OVER_NO_PREMUL:            setBlendModeOverNoPremul(&blend_attachment); break;
        case ONYX_BLEND_MODE_ERASE:                     setBlendModeErase(&blend_attachment); break;
        case ONYX_BLEND_MODE_OVER_MONOCHROME:           setBlendModeOverMonochrome(&blend_attachment); break;
        case ONYX_BLEND_MODE_OVER_NO_PREMUL_MONOCHROME: setBlendModeOverNoPremulMonochrome(&blend_attachment); break;
        case ONYX_BLEND_MODE_ERASE_MONOCHROME:          setBlendModeEraseMonochrome(&blend_attachment); break;
        case ONYX_BLEND_MODE_NONE: break;
    }
    VkPipelineColorBlendStateCreateInfo colorblend = onyx_PipelineColorBlendStateCreateInfo(false, 0, 1, &blend_attachment, NULL);
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    int dynstateCount = dynamicViewport ? 2 : 0;
    VkPipelineDynamicStateCreateInfo dynstate = onyx_PipelineDynamicStateCreateInfo(dynstateCount, dynamicStates);
    VkGraphicsPipelineCreateInfo gpci = onyx_GraphicsPipelineCreateInfo(stageCount, pStages,
            pVertexInputState, &input_assem, &tess, &view, &raster, &multisamp, &depthstencil, 
            &colorblend, &dynstate, layout, renderPass, subpass, VK_NULL_HANDLE, 0);
    V_ASSERT( vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpci, NULL, pipeline) );
};

void
onyx_create_compute_pipeline(VkDevice device, VkPipelineCache cache, const OnyxComputePipelineInfo* s,
                            VkPipeline* pipeline)
{
    VkComputePipelineCreateInfo cpci = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .basePipelineHandle = s->base_pipeline,
        .basePipelineIndex = s->base_pipeline_index,
        .layout = s->layout,
        .flags = s->flags,
        .stage = (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = s->shader_stage.stage,
            .flags = s->shader_stage.flags,
            .module = s->shader_stage.module,
            .pName = s->shader_stage.entry_point,
            .pSpecializationInfo = s->shader_stage.specialization_info
        },
    };

    V_ASSERT( vkCreateComputePipelines(device, cache, 1, &cpci, NULL, pipeline) );
}

const char* 
onyx_GetFullScreenQuadShaderString(void)
{
    return ""
    "#version 460\n"
    "layout(location = 0) out vec2 outUV;\n"
    "// vertex ordering is clockwise\n"
    "void main()\n"
    "{\n"
    "    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);\n"
    "    gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);\n"
    "}\n";
}
