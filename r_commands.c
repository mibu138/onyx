#include "r_commands.h"
#include "m_math.h"
#include "r_render.h"
#include "utils.h"
#include "v_video.h"
#include "r_pipeline.h"
#include "v_memory.h"
#include "def.h"
#include "r_raytrace.h"
#include <memory.h>
#include <assert.h>
#include <string.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_core.h>

typedef struct {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    Mat4 viewInv;
    Mat4 projInv;
} UboMatrices;

static V_block*      matrixBlock;
static UboMatrices*  matrices;
static const Mesh*   hapiMesh;
static V_block* stbBlock;

static RtPushConstants pushConstants;

static void initDescriptors(void)
{
    matrixBlock = v_RequestBlock(sizeof(UboMatrices), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    matrices = (UboMatrices*)matrixBlock->address;
    matrices->model   = m_Ident_Mat4();
    matrices->view    = m_Ident_Mat4();
    matrices->proj    = m_Ident_Mat4();
    matrices->viewInv = m_Ident_Mat4();
    matrices->projInv = m_Ident_Mat4();

    VkDescriptorBufferInfo uniformInfo = {
        .range  =  matrixBlock->size,
        .offset =  matrixBlock->vOffset,
        .buffer = *matrixBlock->vBuffer,
    };

#if RAY_TRACE
    VkWriteDescriptorSetAccelerationStructureKHR asInfo = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures    = &topLevelAS
    };

    VkDescriptorImageInfo imageInfo = {
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .imageView   = offscreenFrameBuffer.colorAttachment.view,
        .sampler     = offscreenFrameBuffer.colorAttachment.sampler
    };
#endif

    VkDescriptorBufferInfo vertBufInfo = {
        .offset = hapiMesh->vertexBlock->vOffset,
        .range  = hapiMesh->vertexBlock->size,
        .buffer = *hapiMesh->vertexBlock->vBuffer,
    };

    VkDescriptorBufferInfo indexBufInfo = {
        .offset =  hapiMesh->indexBlock->vOffset,
        .range  =  hapiMesh->indexBlock->size,
        .buffer = *hapiMesh->indexBlock->vBuffer,
    };

    VkWriteDescriptorSet writes[] = {{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstArrayElement = 0,
            .dstSet = descriptorSets[R_DESC_SET_RASTER],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &uniformInfo
        },{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstArrayElement = 0,
            .dstSet = descriptorSets[R_DESC_SET_RASTER],
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &vertBufInfo
        },{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstArrayElement = 0,
            .dstSet = descriptorSets[R_DESC_SET_RASTER],
            .dstBinding = 2,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &indexBufInfo
        },{
#if RAY_TRACE
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstArrayElement = 0,
            .dstSet = descriptorSets[R_DESC_SET_RAYTRACE],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .pNext = &asInfo
        },{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstArrayElement = 0,
            .dstSet = descriptorSets[R_DESC_SET_RAYTRACE],
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &imageInfo
        },{
#endif
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstArrayElement = 0,
            .dstSet = descriptorSets[R_DESC_SET_POST],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo
    }};

    vkUpdateDescriptorSets(device, ARRAY_SIZE(writes), writes, 0, NULL);
}

static void rayTrace(const VkCommandBuffer* cmdBuf)
{
    vkCmdBindPipeline(*cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelines[R_PIPELINE_RAYTRACE]);

    vkCmdBindDescriptorSets(*cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
            pipelineLayoutRayTrace, 0, 2, descriptorSets, 0, NULL);

    vkCmdPushConstants(*cmdBuf, pipelineLayoutRayTrace, 
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | 
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
        VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(RtPushConstants), &pushConstants);

    const VkPhysicalDeviceRayTracingPropertiesKHR rtprops = v_GetPhysicalDeviceRayTracingProperties();
    const VkDeviceSize progSize = rtprops.shaderGroupBaseAlignment;
    const VkDeviceSize sbtSize = rtprops.shaderGroupBaseAlignment * 4;
    const VkDeviceSize baseAlignment = stbBlock->vOffset;
    const VkDeviceSize rayGenOffset   = baseAlignment;
    const VkDeviceSize missOffset     = baseAlignment + 1u * progSize;
    const VkDeviceSize hitGroupOffset = baseAlignment + 3u * progSize; // have to jump over 2 miss shaders

    printf("Prog\n");

    assert( rayGenOffset % rtprops.shaderGroupBaseAlignment == 0 );

    const VkStridedBufferRegionKHR raygenShaderBindingTable = {
        .buffer = *stbBlock->vBuffer,
        .offset = rayGenOffset,
        .stride = progSize,
        .size   = sbtSize,
    };

    const VkStridedBufferRegionKHR missShaderBindingTable = {
        .buffer = *stbBlock->vBuffer,
        .offset = missOffset,
        .stride = progSize,
        .size   = sbtSize,
    };

    const VkStridedBufferRegionKHR hitShaderBindingTable = {
        .buffer = *stbBlock->vBuffer,
        .offset = hitGroupOffset,
        .stride = progSize,
        .size   = sbtSize,
    };

    const VkStridedBufferRegionKHR callableShaderBindingTable = {
    };

    vkCmdTraceRaysKHR(*cmdBuf, &raygenShaderBindingTable, &missShaderBindingTable, &hitShaderBindingTable, &callableShaderBindingTable, WINDOW_WIDTH, WINDOW_WIDTH, 1);

    printf("Raytrace recorded!\n");
}

static void rasterize(const VkCommandBuffer* cmdBuf, const VkRenderPassBeginInfo* rpassInfo)
{
    vkCmdBindPipeline(*cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[R_PIPELINE_RASTER]);

    vkCmdBindDescriptorSets(
        *cmdBuf, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipelineLayoutRaster, 
        0, 1, &descriptorSets[R_DESC_SET_RASTER], 
        0, NULL);

    vkCmdBeginRenderPass(*cmdBuf, rpassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (hapiMesh)
    {
        VkBuffer vertBuffers[3] = {
            *hapiMesh->vertexBlock->vBuffer,
            *hapiMesh->vertexBlock->vBuffer,
            *hapiMesh->vertexBlock->vBuffer
        };

        const VkDeviceSize vertOffsets[3] = {
            hapiMesh->posOffset, 
            hapiMesh->colOffset,
            hapiMesh->normOffset
        };

        vkCmdBindVertexBuffers(
            *cmdBuf, 0, ARRAY_SIZE(vertOffsets), 
            vertBuffers, vertOffsets);

        vkCmdBindIndexBuffer(
            *cmdBuf, 
            *hapiMesh->indexBlock->vBuffer, 
            hapiMesh->indexBlock->vOffset, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(*cmdBuf, 
            hapiMesh->indexCount, 1, 0, 
            hapiMesh->vertexBlock->vOffset, 0);
    }

    vkCmdEndRenderPass(*cmdBuf);
}

static void postProc(const VkCommandBuffer* cmdBuf, const VkRenderPassBeginInfo* rpassInfo)
{
    vkCmdBindPipeline(*cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[R_PIPELINE_POST]);

    vkCmdBindDescriptorSets(
        *cmdBuf, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipelineLayoutPostProcess, 
        0, 1, &descriptorSets[R_DESC_SET_POST],
        0, NULL);

    vkCmdBeginRenderPass(*cmdBuf, rpassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdDraw(*cmdBuf, 3, 1, 0, 0);

    vkCmdEndRenderPass(*cmdBuf);
}

void r_InitRenderCommands(void)
{
    assert(hapiMesh);
    initDescriptors();

    pushConstants.clearColor = (Vec4){0.1, 0.2, 0.5, 1.0};
    pushConstants.lightIntensity = 1.0;
    pushConstants.lightDir = (Vec3){-0.707106769, -0.5, -0.5};
    pushConstants.lightType = 0;
    pushConstants.colorOffset =  hapiMesh->colOffset / sizeof(Vec3);
    pushConstants.normalOffset = hapiMesh->normOffset / sizeof(Vec3);

    for (int i = 0; i < FRAME_COUNT; i++) 
    {
        r_RequestFrame();
        
        r_UpdateRenderCommands();

        r_PresentFrame();
    }
}

void r_UpdateRenderCommands(void)
{
    VkResult r;
    Frame* frame = &frames[curFrameIndex];
    vkResetCommandPool(device, frame->commandPool, 0);
    VkCommandBufferBeginInfo cbbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    r = vkBeginCommandBuffer(frame->commandBuffer, &cbbi);

    VkClearValue clearValueColor = {0.002f, 0.023f, 0.009f, 1.0f};
    VkClearValue clearValueDepth = {1.0, 0};

    VkClearValue clears[] = {clearValueColor, clearValueDepth};

    const VkRenderPassBeginInfo rpassOffscreen = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 2,
        .pClearValues = clears,
        .renderArea = {{0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}},
        .renderPass =  *offscreenFrameBuffer.pRenderPass,
        .framebuffer = offscreenFrameBuffer.handle,
    };

    const VkRenderPassBeginInfo rpassSwap = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 1,
        .pClearValues = clears,
        .renderArea = {{0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}},
        .renderPass =  *frame->renderPass,
        .framebuffer = frame->frameBuffer 
    };

    //vkCmdBeginRenderPass(frame->commandBuffer, &rpassOffscreen, VK_SUBPASS_CONTENTS_INLINE);
    //vkCmdEndRenderPass(frame->commandBuffer);

    if (parms.mode == MODE_RAY)
        rayTrace(&frame->commandBuffer);
    else 
        rasterize(&frame->commandBuffer, &rpassOffscreen);

    postProc(&frame->commandBuffer, &rpassSwap);

    r = vkEndCommandBuffer(frame->commandBuffer);
    assert ( VK_SUCCESS == r );
}

void r_CreateShaderBindingTable(void)
{
    const VkPhysicalDeviceRayTracingPropertiesKHR rtprops = v_GetPhysicalDeviceRayTracingProperties();
    const uint32_t groupCount = 4;
    const uint32_t groupHandleSize = rtprops.shaderGroupHandleSize;
    const uint32_t baseAlignment = rtprops.shaderGroupBaseAlignment;
    const uint32_t sbtSize = groupCount * baseAlignment; // 3 shader groups: raygen, miss, closest hit

    uint8_t shaderHandleData[sbtSize];

    printf("ShaderGroup base alignment: %d\n", baseAlignment);
    printf("ShaderGroups total size   : %d\n", sbtSize);

    VkResult r;
    r = vkGetRayTracingShaderGroupHandlesKHR(device, pipelines[R_PIPELINE_RAYTRACE], 0, groupCount, sbtSize, shaderHandleData);
    assert( VK_SUCCESS == r );
    stbBlock = v_RequestBlockAligned(sbtSize, baseAlignment);

    uint8_t* pSrc    = shaderHandleData;
    uint8_t* pTarget = stbBlock->address;

    for (int i = 0; i < groupCount; i++) 
    {
        memcpy(pTarget, pSrc + i * groupHandleSize, groupHandleSize);
        pTarget += baseAlignment;
    }

    printf("Created shader binding table\n");
}

Mat4* r_GetXform(r_XformType type)
{
    switch (type) 
    {
        case R_XFORM_MODEL:    return &matrices->model;
        case R_XFORM_VIEW:     return &matrices->view;
        case R_XFORM_PROJ:     return &matrices->proj;
        case R_XFORM_VIEW_INV: return &matrices->viewInv;
        case R_XFORM_PROJ_INV: return &matrices->projInv;
    }
    return NULL;
}

void r_LoadMesh(const Mesh* mesh)
{
    hapiMesh = mesh;
}
