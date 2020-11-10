#include "t_def.h"
#include "r_render.h"
#include "v_video.h"
#include "v_memory.h"
#include "r_pipeline.h"
#include "r_raytrace.h"
#include "t_utils.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>


VkRenderPass   swapchainRenderPass;
VkRenderPass   offscreenRenderPass;
VkRenderPass   msaaRenderPass;

// TODO: we should implement a way to specify the offscreen format
const VkFormat presentColorFormat   = VK_FORMAT_R8G8B8A8_SRGB;
const VkFormat offscreenColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
const VkFormat depthFormat          = VK_FORMAT_D32_SFLOAT;

Tanto_R_Frame  frames[TANTO_FRAME_COUNT];
uint32_t       curFrameIndex;

static void initFrames(void)
{
    const VkCommandPoolCreateInfo cmdPoolCi = {
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };

    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        V_ASSERT( vkCreateCommandPool(device, &cmdPoolCi, NULL, &frames[i].commandPool) );

        const VkCommandBufferAllocateInfo allocInfo = {
            .commandBufferCount = 1,
            .commandPool = frames[i].commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
        };

        V_ASSERT( vkAllocateCommandBuffers(device, &allocInfo, &frames[i].commandBuffer) );
        // spec states that the last parm is an array of commandBuffers... hoping a pointer
        // to a single one works just as well

        const VkSemaphoreCreateInfo semaCi = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        V_ASSERT( vkCreateSemaphore(device, &semaCi, NULL, &frames[i].semaphore) );

        const VkFenceCreateInfo fenceCi = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        V_ASSERT( vkCreateFence(device, &fenceCi, NULL, &frames[i].fence) );

        frames[i].index = i;
        frames[i].swapImage.handle = swapchainImages[i];

        VkImageSubresourceRange ssr = {
            .baseArrayLayer = 0,
            .layerCount = 1,
            .baseMipLevel = 0,
            .levelCount = 1,
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        };

        VkImageViewCreateInfo imageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .subresourceRange = ssr,
            .format = swapFormat,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .image = frames[i].swapImage.handle,
        };

        V_ASSERT( vkCreateImageView(device, &imageViewInfo, NULL, &frames[i].swapImage.view) );
    }
    V1_PRINT("Frames successfully initialized.\n");
}

static void initRenderPassesSwapOff(void)
{
    const VkAttachmentDescription attachmentColor = {
        .format = offscreenColorFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT, // TODO look into what this means
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    const VkAttachmentDescription attachmentDepth = {
        .format = depthFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT, // TODO look into what this means
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentReference referenceColor = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentReference referenceDepth = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &referenceColor,
        .pDepthStencilAttachment = &referenceDepth,
        .inputAttachmentCount = 0,
        .preserveAttachmentCount = 0,
    };

    const VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkAttachmentDescription attachments[] = {
        attachmentColor,
        attachmentDepth,
    };

    VkRenderPassCreateInfo ci = {
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .attachmentCount = TANTO_ARRAY_SIZE(attachments),
        .pAttachments = attachments,
        .dependencyCount = 1,
        .pDependencies = &dependency,
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    };

    V_ASSERT( vkCreateRenderPass(device, &ci, NULL, &offscreenRenderPass) );

    subpass.pDepthStencilAttachment = NULL;
    ci.attachmentCount = 1;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].format      = swapFormat;

    V_ASSERT( vkCreateRenderPass(device, &ci, NULL, &swapchainRenderPass) );
}

static void initRenderPassMSAA(VkSampleCountFlags sampleCount)
{
    const VkAttachmentDescription attachmentColor = {
        .format = swapFormat,
        .samples = sampleCount, // TODO look into what this means
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    const VkAttachmentDescription attachmentDepth = {
        .format = depthFormat,
        .samples = sampleCount, // TODO look into what this means
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentDescription attachmentPresent = {
        .format = swapFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT, // TODO look into what this means
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentDescription attachments[] = {
        attachmentColor,
        attachmentDepth,
        attachmentPresent
    };

    const VkAttachmentReference referenceColor = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentReference referenceDepth = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentReference referenceResolve = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pResolveAttachments  = &referenceResolve,
        .pColorAttachments    = &referenceColor,
        .pDepthStencilAttachment = &referenceDepth,
        .inputAttachmentCount = 0,
        .preserveAttachmentCount = 0,
    };

    const VkSubpassDependency dependency0 = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    const VkSubpassDependency dependency1 = {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkSubpassDependency dependencies[] = {
        dependency0, dependency1
    };

    VkRenderPassCreateInfo ci = {
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .attachmentCount = TANTO_ARRAY_SIZE(attachments),
        .pAttachments = attachments,
        .dependencyCount = 2,
        .pDependencies = dependencies,
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    };

    V_ASSERT( vkCreateRenderPass(device, &ci, NULL, &msaaRenderPass) );
}

static void initRenderPasses(void)
{
    initRenderPassesSwapOff();
    initRenderPassMSAA(VK_SAMPLE_COUNT_8_BIT);
}

void tanto_r_Init(void)
{
    curFrameIndex = 0;
    initRenderPasses();
    initFrames();
    if (tanto_v_config.rayTraceEnabled)
        tanto_r_InitRayTracing();
}

void tanto_r_WaitOnQueueSubmit(void)
{
    vkWaitForFences(device, 1, &frames[curFrameIndex].fence, VK_TRUE, UINT64_MAX);
}

Tanto_R_Frame* tanto_r_RequestFrame(void)
{
    uint32_t i = frameCounter % TANTO_FRAME_COUNT;
    V_ASSERT( vkAcquireNextImageKHR(device, 
            swapchain, 
            UINT64_MAX, 
            imageAcquiredSemaphores[i], 
            VK_NULL_HANDLE, 
            &curFrameIndex) );
    frameCounter++;
    return &frames[curFrameIndex];
}

void tanto_r_PresentFrame(void)
{
    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pWaitDstStageMask = &stageFlags,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAcquiredSemaphores[curFrameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &frames[curFrameIndex].semaphore,
        .commandBufferCount = 1,
        .pCommandBuffers = &frames[curFrameIndex].commandBuffer,
    };

    vkWaitForFences(device, 1, &frames[curFrameIndex].fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frames[curFrameIndex].fence);

    V_ASSERT( vkQueueSubmit(graphicsQueues[0], 1, &si, frames[curFrameIndex].fence) );

    VkResult r;
    const VkPresentInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frames[curFrameIndex].semaphore,
        .pResults = &r,
        .pImageIndices = &curFrameIndex,
    };

    V_ASSERT( vkQueuePresentKHR(presentQueue, &info) );
    assert( VK_SUCCESS == r );
}

void tanto_r_CleanUp(void)
{
    if (tanto_v_config.rayTraceEnabled)
        tanto_r_RayTraceCleanUp();
    tanto_r_CleanUpPipelines();
    vkDestroyRenderPass(device, swapchainRenderPass, NULL);
    vkDestroyRenderPass(device, offscreenRenderPass, NULL);
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        vkDestroyFence(device, frames[i].fence, NULL);
        //vkDestroyImageView(device, frames[i].frameBuffer.colorAttachment.view, NULL);
        //vkDestroyFramebuffer(device, frames[i].frameBuffer.handle, NULL);
        vkDestroySemaphore(device, frames[i].semaphore, NULL);
        vkDestroyCommandPool(device, frames[i].commandPool, NULL);
    }
}
