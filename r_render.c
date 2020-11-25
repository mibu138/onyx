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
#include <vulkan/vulkan_core.h>


VkRenderPass   swapchainRenderPass;
VkRenderPass   offscreenRenderPass;
VkRenderPass   msaaRenderPass;

// TODO: we should implement a way to specify the offscreen format
const VkFormat presentColorFormat   = VK_FORMAT_R8G8B8A8_SRGB;
const VkFormat offscreenColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
const VkFormat depthFormat          = VK_FORMAT_D32_SFLOAT;

static Tanto_R_Frame  frames[TANTO_FRAME_COUNT];
static uint32_t     curFrameIndex;

static VkSwapchainKHR      swapchain;
static VkImage             swapchainImages[TANTO_FRAME_COUNT];
const VkFormat      swapFormat = VK_FORMAT_B8G8R8A8_SRGB;

static VkSemaphore  imageAcquiredSemaphores[TANTO_FRAME_COUNT];
static uint8_t      imageAcquiredSemaphoreIndex = 0;
static uint64_t            frameCounter;
  
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {

        VkExtent2D actualExtent = {
            TANTO_WINDOW_WIDTH,
            TANTO_WINDOW_HEIGHT
        };

        actualExtent.width =  MAX(capabilities.minImageExtent.width,  MIN(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = MAX(capabilities.minImageExtent.height, MIN(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

static void initSwapchain(void)
{
    VkBool32 supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, *pSurface, &supported);

    assert(supported == VK_TRUE);

    VkSurfaceCapabilitiesKHR capabilities;
    V_ASSERT( vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, *pSurface, &capabilities) );

    uint32_t formatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, *pSurface, &formatsCount, NULL);
    VkSurfaceFormatKHR surfaceFormats[formatsCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, *pSurface, &formatsCount, surfaceFormats);

    V1_PRINT("Surface formats: \n");
    for (int i = 0; i < formatsCount; i++) {
        V1_PRINT("Format: %d   Colorspace: %d\n", surfaceFormats[i].format, surfaceFormats[i].colorSpace);
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, *pSurface, &presentModeCount, NULL);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, *pSurface, &presentModeCount, presentModes);

    //const VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // i already know its supported 
    const VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // I get less input lag with this mode

    assert(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    V1_PRINT("Surface Capabilities: Min swapchain image count: %d\n", capabilities.minImageCount);

    const VkExtent2D extent = chooseSwapExtent(capabilities);
    TANTO_WINDOW_WIDTH =  extent.width;
    TANTO_WINDOW_HEIGHT = extent.height;

    const VkSwapchainCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = *pSurface,
        .minImageCount = 2,
        .imageFormat = swapFormat, //50
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1, // number of views in a multiview / stereo surface
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // queue sharing. see vkspec section 11.7. 
        .queueFamilyIndexCount = 0, // dont need with exclusive sharing
        .pQueueFamilyIndices = NULL, // ditto
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, //dunno. may affect blending
        .presentMode = presentMode,
        .clipped = VK_FALSE, // allows pixels convered by another window to be clipped. but will mess up saving the swap image.
        .oldSwapchain = VK_NULL_HANDLE
    };

    V_ASSERT( vkCreateSwapchainKHR(device, &ci, NULL, &swapchain) );

    uint32_t imageCount;
    V_ASSERT( vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL) );
    assert(TANTO_FRAME_COUNT == imageCount);
    V_ASSERT( vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages) );

    V1_PRINT("Swapchain created successfully.\n");
}

static void initSwapchainSemaphores(void)
{
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        VkSemaphoreCreateInfo semaCi = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        V_ASSERT( vkCreateSemaphore(device, &semaCi, NULL, &imageAcquiredSemaphores[i]) );
        printf("Created swapchain semaphore: %p \n", imageAcquiredSemaphores[i]);
    }
}

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
        printf("Created frame %d semaphore %p \n", i, frames[i].semaphore);

        const VkFenceCreateInfo fenceCi = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        V_ASSERT( vkCreateFence(device, &fenceCi, NULL, &frames[i].fence) );

        frames[i].index = i;
    }
    V1_PRINT("Frames successfully initialized.\n");
}

static void bindFramesToSwapImages(void)
{
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
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
    frameCounter  = 0;
    initSwapchain();
    initSwapchainSemaphores();
    initRenderPasses();
    initFrames();
    bindFramesToSwapImages();
    if (tanto_v_config.rayTraceEnabled)
        tanto_r_InitRayTracing();
}

void tanto_r_WaitOnQueueSubmit(void)
{
    vkWaitForFences(device, 1, &frames[curFrameIndex].fence, VK_TRUE, UINT64_MAX);
}

const int8_t tanto_r_RequestFrame(void)
{
    //const uint32_t i = frameCounter % TANTO_FRAME_COUNT;
    imageAcquiredSemaphoreIndex = frameCounter % TANTO_FRAME_COUNT;
    VkResult r;
    r = vkAcquireNextImageKHR(device, 
            swapchain, 
            UINT64_MAX, 
            imageAcquiredSemaphores[imageAcquiredSemaphoreIndex], 
            VK_NULL_HANDLE, 
            &curFrameIndex);
    if (VK_ERROR_OUT_OF_DATE_KHR == r) return -1;
    frameCounter++;
    return curFrameIndex;
}

bool tanto_r_PresentFrame(void)
{
    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pWaitDstStageMask = &stageFlags,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAcquiredSemaphores[imageAcquiredSemaphoreIndex],
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

    VkResult presentResult;
    presentResult = vkQueuePresentKHR(presentQueue, &info);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        return false;
    assert( VK_SUCCESS == r );
    return true;
}

void tanto_r_RecreateSwapchain(void)
{
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        vkDestroyImageView(device, frames[i].swapImage.view, NULL);   
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    initSwapchain();
    bindFramesToSwapImages();
}

Tanto_R_Frame* tanto_r_GetFrame(const int8_t index)
{
    assert( index >= 0 );
    assert( index < TANTO_FRAME_COUNT );
    return &frames[index];
}

void tanto_r_CleanUp(void)
{
    if (tanto_v_config.rayTraceEnabled)
        tanto_r_RayTraceCleanUp();
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        vkDestroySemaphore(device, imageAcquiredSemaphores[i], NULL);
        vkDestroySemaphore(device, frames[i].semaphore, NULL);
        vkDestroyCommandPool(device, frames[i].commandPool, NULL);
        vkDestroyFence(device, frames[i].fence, NULL);
        vkDestroyImageView(device, frames[i].swapImage.view, NULL);
    }
    tanto_r_CleanUpPipelines();
    vkDestroyRenderPass(device, swapchainRenderPass, NULL);
    vkDestroyRenderPass(device, offscreenRenderPass, NULL);
    vkDestroyRenderPass(device, msaaRenderPass, NULL);
    vkDestroySwapchainKHR(device, swapchain, NULL);
}

VkFormat tanto_r_GetOffscreenColorFormat(void) { return offscreenColorFormat; }
VkFormat tanto_r_GetDepthFormat(void)          { return depthFormat; }
VkFormat tanto_r_GetSwapFormat(void)           { return swapFormat; }

VkRenderPass tanto_r_GetSwapchainRenderPass(void) { return swapchainRenderPass; }
VkRenderPass tanto_r_GetOffscreenRenderPass(void) { return offscreenRenderPass; }
VkRenderPass tanto_r_GetMSAARenderPass(void)      { return msaaRenderPass; }
