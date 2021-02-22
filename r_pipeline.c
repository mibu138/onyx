#include "r_pipeline.h"
#include "r_raytrace.h"
#include "v_video.h"
#include "r_render.h"
#include "t_def.h"
#include "v_vulkan.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_core.h>

enum shaderStageType { VERT, FRAG };

static void setBlendModeOverPremult(VkPipelineColorBlendAttachmentState* state)
{
    state->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state->colorBlendOp = VK_BLEND_OP_ADD;
    state->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    state->dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    state->alphaBlendOp = VK_BLEND_OP_ADD;
}

static void setBlendModeOverStraight(VkPipelineColorBlendAttachmentState* state)
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

static void initShaderModule(const char* filepath, VkShaderModule* module)
{
    int fr;
    FILE* fp;
    fp = fopen(filepath, "rb");
    if (fp == 0)
    {
        printf("Failed to open %s\n", filepath);
        exit(0);
    }
    fr = fseek(fp, 0, SEEK_END);
    if (fr != 0)
    {
        printf("Seek failed on %s\n", filepath);
        exit(0);
    }
    assert( fr == 0 ); // success 
    size_t codeSize = ftell(fp);
    rewind(fp);

    unsigned char code[codeSize];
    fread(code, 1, codeSize, fp);
    fclose(fp);

    const VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode = (uint32_t*)code,
    };

    V_ASSERT( vkCreateShaderModule(device, &shaderInfo, NULL, module) );
}

#define MAX_GP_SHADER_STAGES 4
#define MAX_ATTACHMENT_STATES 8

void obdn_r_CreateGraphicsPipelines(const uint8_t count, const Obdn_R_GraphicsPipelineInfo pipelineInfos[count], 
        VkPipeline pipelines[count])
{
    VkPipelineShaderStageCreateInfo           shaderStages[count][MAX_GP_SHADER_STAGES];
    VkPipelineVertexInputStateCreateInfo      vertexInputStates[count];
    VkPipelineInputAssemblyStateCreateInfo    inputAssemblyStates[count];
    VkPipelineTessellationStateCreateInfo     tessellationStates[count];
    VkPipelineViewportStateCreateInfo         viewportStates[count];
    VkPipelineRasterizationStateCreateInfo    rasterizationStates[count];
    VkPipelineMultisampleStateCreateInfo      multisampleStates[count];
    VkPipelineDepthStencilStateCreateInfo     depthStencilStates[count];
    VkPipelineColorBlendStateCreateInfo       colorBlendStates[count];
    //VkPipelineDynamicStateCreateInfo          dynamicStates[count];

    VkViewport viewports[count];
    VkRect2D   scissors[count];

    VkPipelineColorBlendAttachmentState attachmentStates[count][MAX_ATTACHMENT_STATES];

    VkGraphicsPipelineCreateInfo createInfos[count];

    for (int i = 0; i < count; i++) 
    {
        const Obdn_R_GraphicsPipelineInfo* rasterInfo = &pipelineInfos[i];
        assert( rasterInfo->vertShader && rasterInfo->fragShader ); // must have at least these 2

        VkShaderModule vertModule;
        VkShaderModule fragModule;
        VkShaderModule tessCtrlModule;
        VkShaderModule tessEvalModule;

        initShaderModule(rasterInfo->vertShader, &vertModule);
        initShaderModule(rasterInfo->fragShader, &fragModule);

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
            initShaderModule(rasterInfo->tessCtrlShader, &tessCtrlModule);
            initShaderModule(rasterInfo->tessEvalShader, &tessEvalModule);
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

        OBDN_COND_PRINT(rasterInfo->vertexDescription.attributeCount == 0, 
                "rasterInfo.vertexDescription.attributeCount == 0. Assuming verts defined in shader.");

        vertexInputStates[i] = (VkPipelineVertexInputStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount =   rasterInfo->vertexDescription.bindingCount,
            .pVertexBindingDescriptions =      rasterInfo->vertexDescription.bindingDescriptions,
            .vertexAttributeDescriptionCount = rasterInfo->vertexDescription.attributeCount,
            .pVertexAttributeDescriptions =    rasterInfo->vertexDescription.attributeDescriptions
        };

        inputAssemblyStates[i] = (VkPipelineInputAssemblyStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE // applies only to index calls
        };

        if (rasterInfo->polygonMode == VK_POLYGON_MODE_POINT)
            inputAssemblyStates[i].topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        if (rasterInfo->polygonMode == VK_POLYGON_MODE_LINE)
            inputAssemblyStates[i].topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

        if (rasterInfo->tessCtrlShader)
            inputAssemblyStates[i].topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

        VkExtent2D viewportDim = rasterInfo->viewportDim;
        if (viewportDim.width < 1) // use window size
        {
            viewportDim.width = OBDN_WINDOW_WIDTH;
            viewportDim.height = OBDN_WINDOW_HEIGHT;
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
            .cullMode = rasterInfo->cullMode == 0 ? VK_CULL_MODE_BACK_BIT : rasterInfo->cullMode,
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
                .blendEnable = (rasterInfo->blendMode == OBDN_R_BLEND_MODE_NONE) ? VK_FALSE : VK_TRUE, 
            };
        }

        switch (rasterInfo->blendMode)
        {
            case OBDN_R_BLEND_MODE_OVER:          setBlendModeOverPremult(&attachmentStates[i][0]); break;
            case OBDN_R_BLEND_MODE_OVER_STRAIGHT: setBlendModeOverStraight(&attachmentStates[i][0]); break;
            case OBDN_R_BLEND_MODE_ERASE:         setBlendModeErase(&attachmentStates[i][0]); break;
            default: break;
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

        createInfos[i] = (VkGraphicsPipelineCreateInfo){
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .basePipelineIndex = 0, // not used
            .basePipelineHandle = 0,
            .subpass = rasterInfo->subpass, // which subpass in the renderpass do we use this pipeline with
            .renderPass = rasterInfo->renderPass,
            .layout = rasterInfo->layout,
            .pDynamicState = NULL,
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

void obdn_r_CreateRayTracePipelines(const uint8_t count, const Obdn_R_RayTracePipelineInfo pipelineInfos[count], 
        VkPipeline pipelines[count], Obdn_R_ShaderBindingTable shaderBindingTables[count])
{
    assert(count > 0);
    assert(count < OBDN_MAX_PIPELINES);

    VkRayTracingPipelineCreateInfoKHR createInfos[count];

    VkPipelineShaderStageCreateInfo               shaderStages[count][MAX_RT_SHADER_COUNT];
    VkRayTracingShaderGroupCreateInfoKHR          shaderGroups[count][MAX_RT_SHADER_COUNT];
    VkPipelineLibraryCreateInfoKHR                libraryInfos[count];
    //VkRayTracingPipelineInterfaceCreateInfoKHR    libraryInterfaces[count];
    //VkPipelineDynamicStateCreateInfo              dynamicStates[count];
    
    memset(shaderStages, 0, sizeof(shaderStages));
    memset(shaderGroups, 0, sizeof(shaderGroups));
    memset(libraryInfos, 0, sizeof(libraryInfos));
    memset(createInfos,  0, sizeof(createInfos));

    for (int p = 0; p < count; p++) 
    {
        const Obdn_R_RayTracePipelineInfo* rayTraceInfo = &pipelineInfos[p];

        assert(rayTraceInfo->layout != VK_NULL_HANDLE);

        const uint8_t raygenCount = rayTraceInfo->raygenCount;
        const uint8_t missCount   = rayTraceInfo->missCount;
        const uint8_t chitCount   = rayTraceInfo->chitCount;

        assert( raygenCount == 1 ); // for now
        assert( missCount < 10 );
        assert( chitCount < 10 ); // make sure its in a reasonable range

        VkShaderModule raygenSM[raygenCount];
        VkShaderModule missSM[missCount];
        VkShaderModule chitSM[chitCount];

        memset(raygenSM, 0, sizeof(raygenSM));
        memset(missSM, 0, sizeof(missSM));
        memset(chitSM, 0, sizeof(chitSM));

        char* const* const raygenShaders = rayTraceInfo->raygenShaders;
        char* const* const missShaders =   rayTraceInfo->missShaders;
        char* const* const chitShaders =   rayTraceInfo->chitShaders;

        for (int i = 0; i < raygenCount; i++) 
            initShaderModule(raygenShaders[i], &raygenSM[i]);
        for (int i = 0; i < missCount; i++) 
            initShaderModule(missShaders[i], &missSM[i]);
        for (int i = 0; i < chitCount; i++) 
            initShaderModule(chitShaders[i], &chitSM[i]);

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
        obdn_r_CreateShaderBindingTable(createInfos[i].groupCount, pipelines[i], &shaderBindingTables[i]);
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

void obdn_r_CreateDescriptorSetLayouts(const uint8_t count, const Obdn_R_DescriptorSetInfo sets[count],
        VkDescriptorSetLayout layouts[count])
{
    // counters for different descriptors
    assert( count < OBDN_MAX_DESCRIPTOR_SETS);
    for (int i = 0; i < count; i++) 
    {
        const Obdn_R_DescriptorSetInfo set = sets[i];
        assert(set.bindingCount > 0);
        assert(set.bindingCount <= OBDN_MAX_BINDINGS);
        VkDescriptorBindingFlags bindFlags[set.bindingCount];
        VkDescriptorSetLayoutBinding bindings[set.bindingCount];
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

void obdn_r_CreateDescriptorSets(const uint8_t count, const Obdn_R_DescriptorSetInfo sets[count], 
        const VkDescriptorSetLayout layouts[count],
        Obdn_R_Description* out)
{
    // counters for different descriptors
    assert( count < OBDN_MAX_DESCRIPTOR_SETS);

    out->descriptorSetCount = count;

    int dcUbo = 0, dcAs = 0, dcSi = 0, dcSb = 0, dcCis = 0, dcIa = 0;
    for (int i = 0; i < count; i++) 
    {
        const Obdn_R_DescriptorSetInfo set = sets[i];
        assert(set.bindingCount > 0);
        assert(set.bindingCount <= OBDN_MAX_BINDINGS);
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
        .poolSizeCount = OBDN_ARRAY_SIZE(poolSizes),
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

void obdn_r_CreatePipelineLayouts(const uint8_t count, const Obdn_R_PipelineLayoutInfo layoutInfos[static count], 
        VkPipelineLayout pipelineLayouts[count])
{
    assert(count < OBDN_MAX_PIPELINES);
    for (int i = 0; i < count; i++) 
    {
        const Obdn_R_PipelineLayoutInfo* layoutInfo = &layoutInfos[i];
        assert(layoutInfo->pushConstantCount  < OBDN_MAX_PUSH_CONSTANTS);
        assert(layoutInfo->descriptorSetCount < OBDN_MAX_DESCRIPTOR_SETS);

        //assert(dsCount > 0 && dsCount < OBDN_MAX_DESCRIPTOR_SETS);

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

void obdn_r_CleanUpPipelines()
{
    printf("%s called. no longer does anything\n", __PRETTY_FUNCTION__);
}

char* obdn_r_FullscreenTriVertShader(void)
{
    return OBDN_SPVDIR"/post-vert.spv";
}

void obdn_r_DestroyDescription(Obdn_R_Description* d)
{
    vkDestroyDescriptorPool(device, d->descriptorPool, NULL);
    memset(d, 0, sizeof(*d));
}
