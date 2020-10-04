#include "def.h"
#include "r_render.h"
#include "v_video.h"
#include "v_memory.h"
#include "r_pipeline.h"
#include "r_raytrace.h"
#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <vulkan/vulkan_core.h>


VkRenderPass   swapchainRenderPass;
VkRenderPass   offscreenRenderPass;

const VkFormat presentColorFormat = VK_FORMAT_R8G8B8A8_SRGB;
const VkFormat offscreenColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

Frame          frames[FRAME_COUNT];
uint32_t       curFrameIndex;

static void initFrames(void)
{
    VkResult r;
    const VkCommandPoolCreateInfo cmdPoolCi = {
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };

    for (int i = 0; i < FRAME_COUNT; i++) 
    {
        r = vkCreateCommandPool(device, &cmdPoolCi, NULL, &frames[i].commandPool);
        assert(r == VK_SUCCESS);

        const VkCommandBufferAllocateInfo allocInfo = {
            .commandBufferCount = 1,
            .commandPool = frames[i].commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
        };

        r = vkAllocateCommandBuffers(device, &allocInfo, &frames[i].commandBuffer);
        // spec states that the last parm is an array of commandBuffers... hoping a pointer
        // to a single one works just as well
        assert( VK_SUCCESS == r );

        const VkSemaphoreCreateInfo semaCi = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        r = vkCreateSemaphore(device, &semaCi, NULL, &frames[i].semaphore);
        assert( VK_SUCCESS == r );

        const VkFenceCreateInfo fenceCi = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        r = vkCreateFence(device, &fenceCi, NULL, &frames[i].fence);
        assert( VK_SUCCESS == r );

        frames[i].index = i;
        frames[i].pImage = &swapchainImages[i];

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
            .image = *frames[i].pImage,
        };

        r = vkCreateImageView(device, &imageViewInfo, NULL, &frames[i].imageView);
        assert( VK_SUCCESS == r );

        frames[i].renderPass = &swapchainRenderPass;
    }
    V1_PRINT("Frames successfully initialized.\n");
}

static void initRenderPasses(void)
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
        .attachmentCount = ARRAY_SIZE(attachments),
        .pAttachments = attachments,
        .dependencyCount = 1,
        .pDependencies = &dependency,
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    };

    VkResult r = vkCreateRenderPass(device, &ci, NULL, &offscreenRenderPass);
    assert( VK_SUCCESS == r );

    subpass.pDepthStencilAttachment = NULL;
    ci.attachmentCount = 1;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].format      = swapFormat;

    r = vkCreateRenderPass(device, &ci, NULL, &swapchainRenderPass);
    assert( VK_SUCCESS == r );
}

static void initFrameBuffers(void)
{
    VkResult r;
    for (int i = 0; i < FRAME_COUNT; i++) 
    {
        const VkImageView attachments[] = {
            frames[i].imageView,
        };

        const VkFramebufferCreateInfo ci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .layers = 1,
            .renderPass = *frames->renderPass,
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .attachmentCount = 1,
            .pAttachments = attachments,
        };

        r = vkCreateFramebuffer(device, &ci, NULL, &frames[i].frameBuffer);
        assert( VK_SUCCESS == r );
    }
}

void r_Init(void)
{
    curFrameIndex = 0;
    initRenderPasses();
    initFrames();
    initFrameBuffers();
    initDescriptorSets();
    r_InitRayTracing();
    initPipelines();
}

void r_WaitOnQueueSubmit(void)
{
    vkWaitForFences(device, 1, &frames[curFrameIndex].fence, VK_TRUE, UINT64_MAX);
}

Frame* r_RequestFrame(void)
{
    VkResult r;
    uint32_t i = frameCounter % FRAME_COUNT;
    r = vkAcquireNextImageKHR(device, 
            swapchain, 
            UINT64_MAX, 
            imageAcquiredSemaphores[i], 
            VK_NULL_HANDLE, 
            &curFrameIndex);
    assert(VK_SUCCESS == r);
    frameCounter++;
    return &frames[curFrameIndex];
}

void r_PresentFrame(void)
{
    VkResult res;

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

    res = vkQueueSubmit(graphicsQueues[0], 1, &si, frames[curFrameIndex].fence);
    assert( VK_SUCCESS == res );

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

    res = vkQueuePresentKHR(presentQueue, &info);
    assert( VK_SUCCESS == r );
    assert( VK_SUCCESS == res );
}

void r_CleanUp(void)
{
    r_RayTraceCleanUp();
    cleanUpPipelines();
    vkDestroyRenderPass(device, swapchainRenderPass, NULL);
    vkDestroyRenderPass(device, offscreenRenderPass, NULL);
    for (int i = 0; i < FRAME_COUNT; i++) 
    {
        vkDestroyFence(device, frames[i].fence, NULL);
        vkDestroyImageView(device, frames[i].imageView, NULL);
        vkDestroyFramebuffer(device, frames[i].frameBuffer, NULL);
        vkDestroySemaphore(device, frames[i].semaphore, NULL);
        vkDestroyCommandPool(device, frames[i].commandPool, NULL);
    }
}
