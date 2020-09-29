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

FrameBuffer    offscreenFrameBuffer;

const VkFormat colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
const VkFormat offscreenColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
static const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

static Image depthAttachment;

Frame          frames[FRAME_COUNT];
uint32_t       curFrameIndex = 0;

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

static void initDepthAttachment(void)
{
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent = {WINDOW_WIDTH, WINDOW_HEIGHT, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkResult r;

    r = vkCreateImage(device, &imageInfo, NULL, &depthAttachment.handle);
    assert( VK_SUCCESS == r );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, depthAttachment.handle, &memReqs);

#ifndef NDEBUG
    V1_PRINT("Depth image reqs: \nSize: %ld\nAlignment: %ld\nTypes: ", 
            memReqs.size, memReqs.alignment);
    bitprint(&memReqs.memoryTypeBits, 32);
#endif

    v_BindImageToMemory(depthAttachment.handle, memReqs.size);
    
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depthAttachment.handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .components = {0, 0, 0, 0}, // no swizzling
        .format = depthFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    r = vkCreateImageView(device, &viewInfo, NULL, &depthAttachment.view);
    assert( VK_SUCCESS == r );
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

static void initOffscreenFrameBuffer(void)
{
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = offscreenColorFormat,
        .extent = {WINDOW_WIDTH, WINDOW_HEIGHT, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = 
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkResult r;

    r = vkCreateImage(device, &imageInfo, NULL, &offscreenFrameBuffer.colorAttachment.handle);
    assert( VK_SUCCESS == r );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, offscreenFrameBuffer.colorAttachment.handle, &memReqs);

#ifndef NDEBUG
    V1_PRINT("Offscreen framebuffer reqs: \nSize: %ld\nAlignment: %ld\nTypes: ", 
            memReqs.size, memReqs.alignment);
    bitprint(&memReqs.memoryTypeBits, 32);
#endif

    v_BindImageToMemory(offscreenFrameBuffer.colorAttachment.handle, memReqs.size);

    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = offscreenFrameBuffer.colorAttachment.handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .components = {0, 0, 0, 0}, // no swizzling
        .format = offscreenColorFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    r = vkCreateImageView(device, &viewInfo, NULL, &offscreenFrameBuffer.colorAttachment.view);
    assert( VK_SUCCESS == r );


    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .mipLodBias = 0.0,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .minLod = 0.0,
        .maxLod = 0.0, // must both be 0 when using unnormalizedCoordinates
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE // allow us to window coordinates in frag shader
    };

    r = vkCreateSampler(device, &samplerInfo, NULL, &offscreenFrameBuffer.colorAttachment.sampler);
    assert( VK_SUCCESS == r );

    // seting render pass and depth attachment
    offscreenFrameBuffer.pRenderPass = &offscreenRenderPass;
    offscreenFrameBuffer.depthAttachment = depthAttachment;

    const VkImageView attachments[] = {offscreenFrameBuffer.colorAttachment.view, offscreenFrameBuffer.depthAttachment.view};

    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .layers = 1,
        .height = WINDOW_HEIGHT,
        .width  = WINDOW_WIDTH,
        .renderPass = *offscreenFrameBuffer.pRenderPass,
        .attachmentCount = 2,
        .pAttachments = attachments
    };

    r = vkCreateFramebuffer(device, &framebufferInfo, NULL, &offscreenFrameBuffer.handle);
    assert( VK_SUCCESS == r );

    {
        VkCommandPool cmdPool;

        VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = graphicsQueueFamilyIndex,
        };

        vkCreateCommandPool(device, &poolInfo, NULL, &cmdPool);

        VkCommandBufferAllocateInfo cmdBufInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandBufferCount = 1,
            .commandPool = cmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };

        VkCommandBuffer cmdBuf;

        vkAllocateCommandBuffers(device, &cmdBufInfo, &cmdBuf);

        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(cmdBuf, &beginInfo);

        VkImageSubresourceRange subResRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseArrayLayer = 0,
            .baseMipLevel = 0,
            .levelCount = 1,
            .layerCount = 1,
        };

        VkImageMemoryBarrier imgBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .image = offscreenFrameBuffer.colorAttachment.handle,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .subresourceRange = subResRange,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
        };

        vkCmdPipelineBarrier(cmdBuf, 
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &imgBarrier);

        vkEndCommandBuffer(cmdBuf);

        v_SubmitToQueueWait(&cmdBuf, V_QUEUE_TYPE_GRAPHICS, 0);

        vkDestroyCommandPool(device, cmdPool, NULL);
    }
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

    //initOffscreenFrameBuffer();
}

void r_Init(void)
{
    initRenderPasses();
    initFrames();
    initDepthAttachment();
    initFrameBuffers();
    initOffscreenFrameBuffer();
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
    vkDestroySampler(device, offscreenFrameBuffer.colorAttachment.sampler, NULL);
    vkDestroyFramebuffer(device, offscreenFrameBuffer.handle, NULL);
    vkDestroyImageView(device, offscreenFrameBuffer.colorAttachment.view, NULL);
    vkDestroyImage(device, offscreenFrameBuffer.colorAttachment.handle, NULL);
    vkDestroyImageView(device, depthAttachment.view, NULL);
    vkDestroyImage(device, depthAttachment.handle, NULL);
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
