#include "r_pipeline.h"
#include "v_video.h"
#include "r_render.h"
#include "m_math.h"
#include "t_def.h"
#include "v_vulkan.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

VkPipeline       pipelines[TANTO_MAX_PIPELINES];
VkDescriptorSet  descriptorSets[TANTO_MAX_DESCRIPTOR_SETS];
VkPipelineLayout pipelineLayouts[TANTO_MAX_PIPELINES];

static VkDescriptorSetLayout descriptorSetLayouts[TANTO_MAX_DESCRIPTOR_SETS]; 
static VkDescriptorPool      descriptorPool;

enum shaderStageType { VERT, FRAG };

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

static void createPipelineRayTrace(const Tanto_R_PipelineInfo* plInfo, VkPipeline* pPipeline)
{
    const int raygenCount = plInfo->payload.rayTraceInfo.raygenCount;
    const int missCount   = plInfo->payload.rayTraceInfo.missCount;
    const int chitCount   = plInfo->payload.rayTraceInfo.chitCount;
    assert( raygenCount == 1 ); // for now
    assert( missCount > 0 && missCount < 10 );
    assert( chitCount > 0 && chitCount < 10 ); // make sure its in a reasonable range

    VkShaderModule raygenSM[raygenCount];
    VkShaderModule missSM[missCount];
    VkShaderModule chitSM[chitCount];
    memset(raygenSM, 0, sizeof(raygenSM));
    memset(missSM, 0, sizeof(missSM));
    memset(chitSM, 0, sizeof(chitSM));

    char* const* const raygenShaders = plInfo->payload.rayTraceInfo.raygenShaders;
    char* const* const missShaders =   plInfo->payload.rayTraceInfo.missShaders;
    char* const* const chitShaders =   plInfo->payload.rayTraceInfo.chitShaders;

    for (int i = 0; i < raygenCount; i++) 
        initShaderModule(raygenShaders[i], &raygenSM[i]);
    for (int i = 0; i < missCount; i++) 
        initShaderModule(missShaders[i], &missSM[i]);
    for (int i = 0; i < chitCount; i++) 
        initShaderModule(chitShaders[i], &chitSM[i]);

    const int shaderCount = raygenCount + missCount + chitCount;

    VkPipelineShaderStageCreateInfo      shaderStages[shaderCount];
    VkRayTracingShaderGroupCreateInfoKHR shaderGroups[shaderCount];
    memset(shaderStages, 0, sizeof(shaderStages));
    memset(shaderGroups, 0, sizeof(shaderGroups));

    for (int i = 0; i < shaderCount; i++) 
    {
        shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[i].pName = "main";

        shaderGroups[i].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroups[i].pShaderGroupCaptureReplayHandle = NULL;
        shaderGroups[i].anyHitShader       = VK_SHADER_UNUSED_KHR;
        shaderGroups[i].closestHitShader   = VK_SHADER_UNUSED_KHR;
        shaderGroups[i].generalShader      = VK_SHADER_UNUSED_KHR;
        shaderGroups[i].intersectionShader = VK_SHADER_UNUSED_KHR;

        if (i < raygenCount)
        {
            int m = i - 0;
            shaderStages[i].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            shaderStages[i].module = raygenSM[m];

            shaderGroups[i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroups[i].generalShader = i;

        }
        else if ( i - raygenCount < missCount )
        {
            int m = i - raygenCount;
            shaderStages[i].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            shaderStages[i].module = missSM[m];

            shaderGroups[i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroups[i].generalShader = i;
        }
        else if ( i - raygenCount - missCount < chitCount )
        {
            int m = i - raygenCount - missCount;
            shaderStages[i].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            shaderStages[i].module = chitSM[m];

            shaderGroups[i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            shaderGroups[i].closestHitShader = i;
        }
    }

    VkRayTracingPipelineCreateInfoKHR pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .maxRecursionDepth = 1, 
        .layout     = pipelineLayouts[plInfo->layoutId],
        .libraries  = {VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR},
        .groupCount = TANTO_ARRAY_SIZE(shaderGroups),
        .stageCount = TANTO_ARRAY_SIZE(shaderStages),
        .pGroups    = shaderGroups,
        .pStages    = shaderStages
    };

    V_ASSERT( vkCreateRayTracingPipelinesKHR(device, NULL, 1, &pipelineInfo, NULL, pPipeline) );

    for (int i = 0; i < raygenCount; i++) 
    {
        vkDestroyShaderModule(device, raygenSM[i], NULL);
    }
    for (int i = 0; i < missCount; i++) 
    {
        vkDestroyShaderModule(device, missSM[i], NULL);
    }
    for (int i = 0; i < chitCount; i++) 
    {
        vkDestroyShaderModule(device, chitSM[i], NULL);
    }
}

#define MAX_SHADER_STAGES 4

static void createPipelineRasterization(const Tanto_R_PipelineInfo* plInfo, VkPipeline* pPipeline)
{
    VkShaderModule vertModule;
    VkShaderModule fragModule;
    VkShaderModule tessCtrlModule;
    VkShaderModule tessEvalModule;

    const Tanto_R_PipelineRasterInfo rasterInfo = plInfo->payload.rasterInfo;

    assert( rasterInfo.vertShader && rasterInfo.fragShader ); // must have at least these 2

    initShaderModule(plInfo->payload.rasterInfo.vertShader, &vertModule);
    initShaderModule(plInfo->payload.rasterInfo.fragShader, &fragModule);

    const VkSpecializationInfo shaderSpecialInfo = {
        // TODO
    };

    uint8_t shaderStageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[MAX_SHADER_STAGES] = {0};
    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = NULL;
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = NULL;
    if (rasterInfo.tessCtrlShader)
    {
        assert(rasterInfo.tessEvalShader);
        initShaderModule(rasterInfo.tessCtrlShader, &tessCtrlModule);
        initShaderModule(rasterInfo.tessEvalShader, &tessEvalModule);
        shaderStages[2].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[2].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        shaderStages[2].module = tessCtrlModule;
        shaderStages[2].pName = "main";
        shaderStages[2].pSpecializationInfo = NULL;
        shaderStages[3].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[3].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        shaderStages[3].module = tessEvalModule;
        shaderStages[3].pName = "main";
        shaderStages[3].pSpecializationInfo = NULL;
        shaderStageCount += 2;
    }


    // passing all of these asserts does not garuantee the vertexDescription is correct.
    // it simply increase the likelyhood.

    TANTO_COND_PRINT(rasterInfo.vertexDescription.attributeCount == 0, 
            "rasterInfo.vertexDescription.attributeCount == 0. Assuming verts defined in shader.");

    const VkPipelineVertexInputStateCreateInfo vertexInput = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount =   rasterInfo.vertexDescription.bindingCount,
        .pVertexBindingDescriptions =      rasterInfo.vertexDescription.bindingDescriptions,
        .vertexAttributeDescriptionCount = rasterInfo.vertexDescription.attributeCount,
        .pVertexAttributeDescriptions =    rasterInfo.vertexDescription.attributeDescriptions
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE // applies only to index calls
    };

    if (rasterInfo.polygonMode == VK_POLYGON_MODE_POINT)
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    if (rasterInfo.polygonMode == VK_POLYGON_MODE_LINE)
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

    if (rasterInfo.tessCtrlShader)
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

    const VkViewport viewport = {
        .height = TANTO_WINDOW_HEIGHT,
        .width = TANTO_WINDOW_WIDTH,
        .x = 0,
        .y = 0,
        .minDepth = 0.0,
        .maxDepth = 1.0
    };

    const VkRect2D scissor = {
        .extent = {TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT},
        .offset = {0, 0}
    };

    const VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .scissorCount = 1,
        .pScissors = &scissor,
        .viewportCount = 1,
        .pViewports = &viewport,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE, // dunno
        .rasterizerDiscardEnable = VK_FALSE, // actually discards everything
        .polygonMode = rasterInfo.polygonMode,
        .cullMode = rasterInfo.cullMode == 0 ? VK_CULL_MODE_BACK_BIT : rasterInfo.cullMode,
        .frontFace = rasterInfo.frontFace,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0
    };

    const VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = rasterInfo.sampleCount 
        // TODO: alot more settings here. more to look into
    };

    const VkPipelineColorBlendAttachmentState attachmentState = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, /* need this to actually
                                                                    write anything to the
                                                                    framebuffer */
        .blendEnable = (rasterInfo.blendMode == TANTO_R_BLEND_MODE_OVER) ? VK_TRUE : VK_FALSE, // no blending for now
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = 0,
        .dstAlphaBlendFactor = 0,
        .alphaBlendOp = 0,
    };

    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE, // only for integer framebuffer formats
        .logicOp = 0,
        .attachmentCount = 1,
        .pAttachments = &attachmentState /* must have independentBlending device   
            feature enabled for these to be different. each entry would correspond 
            to the blending for a different framebuffer. */
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE, // allows you to only keep fragments within the depth bounds
        .stencilTestEnable = VK_FALSE,
    };

    const VkPipelineTessellationStateCreateInfo tesselationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = rasterInfo.tesselationPatchPoints,
    };

    assert(rasterInfo.renderPass != 0);
    assert(shaderStageCount <= MAX_SHADER_STAGES);
    assert(shaderStageCount > 0);

    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .basePipelineIndex = 0, // not used
        .basePipelineHandle = 0,
        .subpass = 0, // which subpass in the renderpass do we use this pipeline with
        .renderPass = rasterInfo.renderPass,
        .layout = pipelineLayouts[plInfo->layoutId],
        .pDynamicState = NULL,
        .pColorBlendState = &colorBlendState,
        .pDepthStencilState = &depthStencilState,
        .pMultisampleState = &multisampleState,
        .pRasterizationState = &rasterizationState,
        .pViewportState = &viewportState,
        .pTessellationState = &tesselationState, // may be able to do splines with this
        .flags = 0,
        .stageCount = shaderStageCount,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
    };

    V_ASSERT( vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pPipeline) );

    vkDestroyShaderModule(device, vertModule, NULL);
    vkDestroyShaderModule(device, fragModule, NULL);
}

void tanto_r_InitDescriptorSets(const Tanto_R_DescriptorSet* const sets, const int count)
{
    // counters for different descriptors
    int dcUbo = 0, dcAs = 0, dcSi = 0, dcSb = 0, dcCis = 0;
    for (int i = 0; i < count; i++) 
    {
        const Tanto_R_DescriptorSet set = sets[i];
        assert(set.bindingCount > 0);
        assert(set.bindingCount <= TANTO_MAX_BINDINGS);
        assert(descriptorSets[set.id] == VK_NULL_HANDLE);
        assert(set.id == i); // we ensure that the set ids increase from with i from 0. No gaps.
        VkDescriptorSetLayoutBinding bindings[set.bindingCount];
        for (int b = 0; b < set.bindingCount; b++) 
        {
            const uint32_t dCount = set.bindings[b].descriptorCount;
            bindings[b].binding = b;
            bindings[b].descriptorCount = dCount;
            bindings[b].descriptorType  = set.bindings[b].type;
            bindings[b].stageFlags      = set.bindings[b].stageFlags;
            bindings[b].pImmutableSamplers = NULL;
            switch (set.bindings[b].type) 
            {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             dcUbo += dCount; break;
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: dcAs  += dCount; break;
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:              dcSi  += dCount; break;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:             dcSb  += dCount; break;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:     dcCis += dCount; break;
                default: assert(false);
            }
        }
        const VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = set.bindingCount,
            .pBindings = bindings
        };
        V_ASSERT(vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayouts[set.id]));
    }

    // were not allowed to specify a count of 0
    dcUbo = dcUbo > 0 ? dcUbo : 1;
    dcAs  = dcAs > 0  ? dcAs  : 1;
    dcSi  = dcSi > 0  ? dcSi  : 1;
    dcSb  = dcSb > 0  ? dcSb  : 1;
    dcCis = dcCis > 0 ? dcCis : 1;

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
    }};

    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = count,
        .poolSizeCount = TANTO_ARRAY_SIZE(poolSizes),
        .pPoolSizes = poolSizes, 
    };

    V_ASSERT(vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool));

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = count,
        .pSetLayouts = descriptorSetLayouts,
    };

    V_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets));
}

void tanto_r_InitPipelineLayouts(const Tanto_R_PipelineLayout *const layouts, const int count)
{
    assert(count > 0 && count < TANTO_MAX_PIPELINES);
    for (int i = 0; i < count; i++) 
    {
        const Tanto_R_PipelineLayout layout = layouts[i];
        assert(layout.id == i);
        assert(layout.pushConstantCount < TANTO_MAX_PUSH_CONSTANTS);

        const int dsCount = layout.descriptorSetCount;
        //assert(dsCount > 0 && dsCount < TANTO_MAX_DESCRIPTOR_SETS);

        VkDescriptorSetLayout descSetLayouts[dsCount];

        for (int j = 0; j < dsCount ; j++) 
        {
            descSetLayouts[j] = descriptorSetLayouts[layout.descriptorSetIds[j]];
        }

        VkPipelineLayoutCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .setLayoutCount = dsCount,
            .pSetLayouts = descSetLayouts,
            .pushConstantRangeCount = layout.pushConstantCount,
            .pPushConstantRanges = layout.pushConstantsRanges
        };

        V_ASSERT(vkCreatePipelineLayout(device, &info, NULL, &pipelineLayouts[layout.id]));
    }
}

void tanto_r_InitPipelines(const Tanto_R_PipelineInfo *const pipelineInfos, const int count)
{
    for (int i = 0; i < count; i++) 
    {
        const Tanto_R_PipelineInfo plInfo = pipelineInfos[i];
        switch (plInfo.type) 
        {
            case TANTO_R_PIPELINE_RASTER_TYPE:   createPipelineRasterization(&plInfo, &pipelines[plInfo.id]); break;
            case TANTO_R_PIPELINE_RAYTRACE_TYPE: createPipelineRayTrace(&plInfo, &pipelines[plInfo.id]); break;
        }
    }
}

void tanto_r_CreatePipeline(const Tanto_R_PipelineInfo* const pipelineInfo, VkPipeline* pipeline)
{
    switch (pipelineInfo->type) 
    {
        case TANTO_R_PIPELINE_RASTER_TYPE: createPipelineRasterization(pipelineInfo, pipeline); break;
        case TANTO_R_PIPELINE_RAYTRACE_TYPE: createPipelineRayTrace(pipelineInfo, pipeline); break;
        default: break;
    }
}

void tanto_r_CleanUpPipelines()
{
    for (int i = 0; i < TANTO_MAX_PIPELINES; i++) 
    {
        if (pipelineLayouts[i])
            vkDestroyPipelineLayout(device, pipelineLayouts[i], NULL);
        pipelineLayouts[i] = 0;
    }
    vkDestroyDescriptorPool(device, descriptorPool, NULL);
    descriptorPool = 0;
    for (int i = 0; i < TANTO_MAX_DESCRIPTOR_SETS; i++) 
    {
        if (descriptorSetLayouts[i])
            vkDestroyDescriptorSetLayout(device, descriptorSetLayouts[i], NULL);
        descriptorSetLayouts[i] = 0;
        descriptorSets[i] = 0;
    }
    for (int i = 0; i < TANTO_MAX_PIPELINES; i++) 
    {
        if (pipelines[i])
            vkDestroyPipeline(device, pipelines[i], NULL);
        pipelines[i] = 0;
    }
}

char* tanto_r_FullscreenTriVertShader(void)
{
    return TANTO_SPVDIR"/post-vert.spv";
}
