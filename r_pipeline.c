#include "r_pipeline.h"
#include "v_video.h"
#include "r_render.h"
#include "r_commands.h"
#include "m_math.h"
#include "def.h"
#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_core.h>

#define PIPELINE_COUNT 3
#define DESCRIPTOR_SET_COUNT 3
#define MAX_DESC_SET_LAYOUTS 3

VkPipeline      pipelines[MAX_PIPELINES];
VkDescriptorSet descriptorSets[MAX_DESCRIPTOR_SETS];

VkPipelineLayout pipelineLayoutRaster;
VkPipelineLayout pipelineLayoutRayTrace;
VkPipelineLayout pipelineLayoutPostProcess;

static VkDescriptorSetLayout descriptorSetLayouts[MAX_DESC_SET_LAYOUTS]; 
static VkDescriptorPool      descriptorPool;

enum shaderStageType { VERT, FRAG };

static void initShaderModule(const char* filepath, VkShaderModule* module)
{
    VkResult r;
    int fr;
    FILE* fp;
    fp = fopen(filepath, "rb");
    fr = fseek(fp, 0, SEEK_END);
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

    r = vkCreateShaderModule(device, &shaderInfo, NULL, module);
    assert( VK_SUCCESS == r );
}

void initDescriptorSets(void)
{
    VkResult r;

    VkDescriptorSetLayoutBinding bindingsRaster[] = {{
            .binding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            .pImmutableSamplers = NULL, // no one really seems to use these
        },{ 
            // will store the vertex buffer for the rt pipeline
            .binding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            .pImmutableSamplers = NULL, // no one really seems to use these
        },{
            // will store the index buffer for the rt pipeline
            .binding = 2,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            .pImmutableSamplers = NULL, // no one really seems to use these
    }};

    VkDescriptorSetLayoutBinding bindingsRayTrace[] = {{
            .binding = 0, 
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
        },{
            .binding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR
    }};

    VkDescriptorSetLayoutBinding bindingsPostProc[] = {{
        .binding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    }};

    VkDescriptorSetLayoutCreateInfo layoutInfoRaster = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = ARRAY_SIZE(bindingsRaster),
        .pBindings = bindingsRaster
    };

    VkDescriptorSetLayoutCreateInfo layoutInfoRayTrace = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = ARRAY_SIZE(bindingsRayTrace),
        .pBindings = bindingsRayTrace
    };

    VkDescriptorSetLayoutCreateInfo layoutInfoPostProc = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = ARRAY_SIZE(bindingsPostProc),
        .pBindings = bindingsPostProc 
    };

    r = vkCreateDescriptorSetLayout(device, &layoutInfoRaster, NULL, &descriptorSetLayouts[R_DESC_SET_RASTER]);
    assert( VK_SUCCESS == r );

    r = vkCreateDescriptorSetLayout(device, &layoutInfoRayTrace, NULL, &descriptorSetLayouts[R_DESC_SET_RAYTRACE]);
    assert( VK_SUCCESS == r );

    r = vkCreateDescriptorSetLayout(device, &layoutInfoPostProc, NULL, &descriptorSetLayouts[R_DESC_SET_POST]);
    assert( VK_SUCCESS == r );

    // these are for specifying how many total descriptors of a 
    // particular type will be allocated from a pool
    VkDescriptorPoolSize poolSize[] = {{
            .descriptorCount = 1,
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        },{
            .descriptorCount = 1,
            .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        },{
            .descriptorCount = 1,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        },{
            .descriptorCount = 2,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        },{
            .descriptorCount = 1,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    }};

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = MAX_DESCRIPTOR_SETS,
        .poolSizeCount = ARRAY_SIZE(poolSize),
        .pPoolSizes = poolSize, 
    };

    r = vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool);
    assert( VK_SUCCESS == r );

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = DESCRIPTOR_SET_COUNT,
        .pSetLayouts = descriptorSetLayouts,
    };

    r = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets);
    assert( VK_SUCCESS == r );
}

static void initPipelineLayouts(void)
{
    VkResult r;
    const VkPipelineLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 1, // ?
        .pSetLayouts = &descriptorSetLayouts[R_DESC_SET_RASTER],
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    r = vkCreatePipelineLayout(device, &info, NULL, &pipelineLayoutRaster);
    assert( VK_SUCCESS == r );

    const VkPipelineLayoutCreateInfo infoPost = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 1, // ?
        .pSetLayouts = &descriptorSetLayouts[R_DESC_SET_POST],
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    r = vkCreatePipelineLayout(device, &infoPost, NULL, &pipelineLayoutPostProcess);
    assert( VK_SUCCESS == r );
}

static void initPipelineRayTrace(void)
{
    VkShaderModule raygenSM;
    VkShaderModule missSM;
    VkShaderModule missShadowSM;
    VkShaderModule chitSM;

    initShaderModule("/home/michaelb/dev/HDK/vk-window/vkcode/shaders/spv/raytrace-rgen.spv",  &raygenSM);
    initShaderModule("/home/michaelb/dev/HDK/vk-window/vkcode/shaders/spv/raytrace-rmiss.spv", &missSM);
    initShaderModule("/home/michaelb/dev/HDK/vk-window/vkcode/shaders/spv/raytraceShadow-rmiss.spv", &missShadowSM);
    initShaderModule("/home/michaelb/dev/HDK/vk-window/vkcode/shaders/spv/raytrace-rchit.spv", &chitSM);

    VkPipelineShaderStageCreateInfo shaderStages[] = {{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .module = raygenSM,
        .pName = "main",
    },{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
        .module = missSM,
        .pName = "main",
    },{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
        .module = missShadowSM,
        .pName = "main",
    },{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        .module = chitSM,
        .pName = "main",
    }};

    VkRayTracingShaderGroupCreateInfoKHR rg = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader =      0, // stage 0 in shaderStages
        .closestHitShader =   VK_SHADER_UNUSED_KHR,
        .anyHitShader =       VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .pShaderGroupCaptureReplayHandle = NULL
    };

    VkRayTracingShaderGroupCreateInfoKHR mg = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader =      1, // stage 1 in shaderStages
        .closestHitShader =   VK_SHADER_UNUSED_KHR,
        .anyHitShader =       VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .pShaderGroupCaptureReplayHandle = NULL
    };

    VkRayTracingShaderGroupCreateInfoKHR msg = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader =      2, // stage 1 in shaderStages
        .closestHitShader =   VK_SHADER_UNUSED_KHR,
        .anyHitShader =       VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .pShaderGroupCaptureReplayHandle = NULL
    };

    VkRayTracingShaderGroupCreateInfoKHR hg = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type  = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        .generalShader =      VK_SHADER_UNUSED_KHR, // stage 1 in shaderStages
        .closestHitShader =   3,
        .anyHitShader =       VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .pShaderGroupCaptureReplayHandle = NULL
    };

    const VkRayTracingShaderGroupCreateInfoKHR shaderGroups[] = {rg, mg, msg, hg};

    const VkPushConstantRange pushConstant = {
        .stageFlags = 
            VK_SHADER_STAGE_RAYGEN_BIT_KHR |
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
            VK_SHADER_STAGE_MISS_BIT_KHR,
        .offset = 0,
        .size = sizeof(RtPushConstants)
    };

    // were reversing the order here to conform to the tutorial
    // const VkDescriptorSetLayout descSetLayouts[] = {descriptorSetLayouts[R_DESC_SET_RAYTRACE], descriptorSetLayouts[R_DESC_SET_RASTER]};

    const VkPipelineLayoutCreateInfo pipelineLayout = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .setLayoutCount = 2,
        .pSetLayouts = descriptorSetLayouts,
        .pPushConstantRanges    = &pushConstant,
    };

    VkResult r;

    r = vkCreatePipelineLayout(device, &pipelineLayout, NULL, &pipelineLayoutRayTrace);
    assert( VK_SUCCESS == r );

    VkRayTracingPipelineCreateInfoKHR pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .maxRecursionDepth = 1, 
        .layout     = pipelineLayoutRayTrace,
        .libraries  = {VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR},
        .groupCount = ARRAY_SIZE(shaderGroups),
        .stageCount = ARRAY_SIZE(shaderStages),
        .pGroups    = shaderGroups,
        .pStages    = shaderStages
    };

    r = vkCreateRayTracingPipelinesKHR(device, NULL, 1, &pipelineInfo, NULL, &pipelines[R_PIPELINE_RAYTRACE]);
    assert( VK_SUCCESS == r );

    vkDestroyShaderModule(device, raygenSM, NULL);
    vkDestroyShaderModule(device, chitSM, NULL);
    vkDestroyShaderModule(device, missSM, NULL);
    vkDestroyShaderModule(device, missShadowSM, NULL);
}

static void initPipelineRaster(void)
{
    initPipelineLayouts();

    VkShaderModule vertModule;
    VkShaderModule fragModule;

    initShaderModule("/home/michaelb/dev/HDK/vk-window/vkcode/shaders/spv/default-vert.spv", &vertModule);
    initShaderModule("/home/michaelb/dev/HDK/vk-window/vkcode/shaders/spv/default-frag.spv", &fragModule);

    const VkSpecializationInfo shaderSpecialInfo = {
        // TODO
    };

    const VkPipelineShaderStageCreateInfo shaderStages[2] = {
        [0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        [0].stage = VK_SHADER_STAGE_VERTEX_BIT,
        [0].module = vertModule,
        [0].pName = "main",
        [0].pSpecializationInfo = NULL,
        [1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        [1].stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        [1].module = fragModule,
        [1].pName = "main",
        [1].pSpecializationInfo = NULL,
    }; // vert and frag

    const VkVertexInputBindingDescription bindingDescriptionPos = {
        .binding = 0,
        .stride  = sizeof(Vec3), 
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    const VkVertexInputBindingDescription bindingDescriptionColor = {
        .binding = 1,
        .stride  = sizeof(Vec3), 
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    const VkVertexInputBindingDescription bindingDescriptionNormal = {
        .binding = 2,
        .stride  = sizeof(Vec3), 
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    const VkVertexInputAttributeDescription positionAttributeDescription = {
        .binding = 0,
        .location = 0, 
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = sizeof(Vec3) * 0
    };

    const VkVertexInputAttributeDescription colorAttributeDescription = {
        .binding = 1,
        .location = 1, 
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = sizeof(Vec3) * 0,
    };

    const VkVertexInputAttributeDescription normalAttributeDescription = {
        .binding = 2,
        .location = 2, 
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = sizeof(Vec3) * 0,
    };

    VkVertexInputBindingDescription vBindDescs[3] = {
        bindingDescriptionPos, bindingDescriptionColor, bindingDescriptionNormal
    };

    VkVertexInputAttributeDescription vAttrDescs[3] = {
        positionAttributeDescription, colorAttributeDescription, normalAttributeDescription 
    };

    const VkPipelineVertexInputStateCreateInfo vertexInput = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = ARRAY_SIZE(vBindDescs),
        .pVertexBindingDescriptions = vBindDescs,
        .vertexAttributeDescriptionCount = ARRAY_SIZE(vAttrDescs),
        .pVertexAttributeDescriptions = vAttrDescs 
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE // applies only to index calls
    };

    const VkViewport viewport = {
        .height = WINDOW_HEIGHT,
        .width = WINDOW_WIDTH,
        .x = 0,
        .y = 0,
        .minDepth = 0.0,
        .maxDepth = 1.0
    };

    const VkRect2D scissor = {
        .extent = {WINDOW_WIDTH, WINDOW_HEIGHT},
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
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0
    };

    const VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        // TODO: alot more settings here. more to look into
    };

    const VkPipelineColorBlendAttachmentState attachmentState = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, /* need this to actually
                                                                    write anything to the
                                                                    framebuffer */
        .blendEnable = VK_FALSE, // no blending for now
        .srcColorBlendFactor = 0,
        .dstColorBlendFactor = 0,
        .colorBlendOp = 0,
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

    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .basePipelineIndex = 0, // not used
        .basePipelineHandle = 0,
        .subpass = 0, // which subpass in the renderpass do we use this pipeline with
        .renderPass = offscreenRenderPass,
        .layout = pipelineLayoutRaster,
        .pDynamicState = NULL,
        .pColorBlendState = &colorBlendState,
        .pDepthStencilState = &depthStencilState,
        .pMultisampleState = &multisampleState,
        .pRasterizationState = &rasterizationState,
        .pViewportState = &viewportState,
        .pTessellationState = NULL, // may be able to do splines with this
        .flags = 0,
        .stageCount = ARRAY_SIZE(shaderStages),
        .pStages = shaderStages,
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
    };

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipelines[R_PIPELINE_RASTER]);

    vkDestroyShaderModule(device, vertModule, NULL);
    vkDestroyShaderModule(device, fragModule, NULL);
}

static void initPipelinePostProc(void)
{
    initPipelineLayouts();

    VkShaderModule vertModule;
    VkShaderModule fragModule;

    initShaderModule("/home/michaelb/dev/HDK/vk-window/vkcode/shaders/spv/post-vert.spv", &vertModule);
    initShaderModule("/home/michaelb/dev/HDK/vk-window/vkcode/shaders/spv/post-frag.spv", &fragModule);

    const VkPipelineShaderStageCreateInfo shaderStages[2] = {
        [0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        [0].stage = VK_SHADER_STAGE_VERTEX_BIT,
        [0].module = vertModule,
        [0].pName = "main",
        [0].pSpecializationInfo = NULL,
        [1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        [1].stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        [1].module = fragModule,
        [1].pName = "main",
        [1].pSpecializationInfo = NULL,
    }; // vert and frag

    const VkPipelineVertexInputStateCreateInfo vertexInput = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE // applies only to index calls
    };

    const VkViewport viewport = {
        .height = WINDOW_HEIGHT,
        .width = WINDOW_WIDTH,
        .x = 0,
        .y = 0,
        .minDepth = 0.0,
        .maxDepth = 1.0
    };

    const VkRect2D scissor = {
        .extent = {WINDOW_WIDTH, WINDOW_HEIGHT},
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
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0
    };

    const VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        // TODO: alot more settings here. more to look into
    };

    const VkPipelineColorBlendAttachmentState attachmentState = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, /* need this to actually
                                                                    write anything to the
                                                                    framebuffer */
        .blendEnable = VK_FALSE, // no blending for now
        .srcColorBlendFactor = 0,
        .dstColorBlendFactor = 0,
        .colorBlendOp = 0,
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
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE, // allows you to only keep fragments within the depth bounds
        .stencilTestEnable = VK_FALSE,
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .basePipelineIndex = 0, // not used
        .basePipelineHandle = 0,
        .subpass = 0, // which subpass in the renderpass do we use this pipeline with
        .renderPass = swapchainRenderPass,
        .layout = pipelineLayoutPostProcess,
        .pDynamicState = NULL,
        .pColorBlendState = &colorBlendState,
        .pDepthStencilState = &depthStencilState,
        .pMultisampleState = &multisampleState,
        .pRasterizationState = &rasterizationState,
        .pViewportState = &viewportState,
        .pTessellationState = NULL, // may be able to do splines with this
        .flags = 0,
        .stageCount = ARRAY_SIZE(shaderStages),
        .pStages = shaderStages,
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
    };

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipelines[R_PIPELINE_POST]);

    vkDestroyShaderModule(device, vertModule, NULL);
    vkDestroyShaderModule(device, fragModule, NULL);
}

void initPipelines(void)
{
    initPipelineRaster();
#if RAY_TRACE
    initPipelineRayTrace();
#endif
    initPipelinePostProc();
}

void cleanUpPipelines()
{
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts[R_DESC_SET_RASTER], NULL);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayouts[R_DESC_SET_RAYTRACE], NULL);
    vkDestroyPipelineLayout(device, pipelineLayoutRaster, NULL);
    vkDestroyPipelineLayout(device, pipelineLayoutPostProcess, NULL);
    for (int i = 0; i < PIPELINE_COUNT; i++) 
    {
        vkDestroyPipeline(device, pipelines[i], NULL);
    }
    vkDestroyDescriptorPool(device, descriptorPool, NULL);
}
