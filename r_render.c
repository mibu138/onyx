#include "t_def.h"
#include "r_render.h"
#include "tanto/r_geo.h"
#include "tanto/v_command.h"
#include "v_video.h"
#include "v_memory.h"
#include "r_pipeline.h"
#include "r_raytrace.h"
#include "t_utils.h"
#include "s_scene.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <vulkan/vulkan_core.h>


// TODO: we should implement a way to specify the offscreen format
const VkFormat presentColorFormat   = VK_FORMAT_R8G8B8A8_SRGB;
const VkFormat offscreenColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
const VkFormat depthFormat          = VK_FORMAT_D32_SFLOAT;
const VkFormat swapFormat           = VK_FORMAT_B8G8R8A8_SRGB;

static Tanto_R_Frame  frames[TANTO_FRAME_COUNT];
static uint32_t     curFrameIndex;

static VkSwapchainKHR      swapchain;
static VkImage             swapchainImages[TANTO_FRAME_COUNT];

static VkSemaphore  imageAcquiredSemaphores[TANTO_FRAME_COUNT];
static uint8_t      imageAcquiredSemaphoreIndex = 0;
static uint64_t            frameCounter;

#define MAX_SWAP_RECREATE_FNS 8
static uint8_t swapRecreateFnCount = 0;
static Tanto_R_SwapchainRecreationFn swapchainRecreationFns[MAX_SWAP_RECREATE_FNS];

static VkSemaphore presentationSemaphores[TANTO_FRAME_COUNT];
  
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
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        frames[i].command = tanto_v_CreateCommand();
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

static void recreateSwapchain(void)
{
    vkDeviceWaitIdle(device);
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        vkDestroyImageView(device, frames[i].swapImage.view, NULL);   
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    initSwapchain();
    bindFramesToSwapImages();
    for (int i = 0; i < swapRecreateFnCount; i++) 
    {
        swapchainRecreationFns[i]();   
    }
}

void tanto_r_Init(void)
{
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        const VkSemaphoreCreateInfo sc = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        vkCreateSemaphore(device, &sc, NULL, &presentationSemaphores[i]);
    }
    curFrameIndex = 0;
    frameCounter  = 0;
    initSwapchain();
    initSwapchainSemaphores();
    initFrames();
    bindFramesToSwapImages();
    if (tanto_v_config.rayTraceEnabled)
        tanto_r_InitRayTracing();
    printf("Tanto Renderer initialized.\n");
}

void tanto_r_WaitOnQueueSubmit(void)
{
    vkWaitForFences(device, 1, &frames[curFrameIndex].command.fence, VK_TRUE, UINT64_MAX);
}

const uint32_t tanto_r_RequestFrame(void)
{
    //const uint32_t i = frameCounter % TANTO_FRAME_COUNT;
    imageAcquiredSemaphoreIndex = frameCounter % TANTO_FRAME_COUNT;
    VkResult r;
retry:
    r = vkAcquireNextImageKHR(device, 
            swapchain, 
            100000, 
            imageAcquiredSemaphores[imageAcquiredSemaphoreIndex], 
            VK_NULL_HANDLE, 
            &curFrameIndex);
    if (VK_ERROR_OUT_OF_DATE_KHR == r) 
    {
        recreateSwapchain();
        goto retry;
    }
    frameCounter++;
    return curFrameIndex;
}

void tanto_r_SubmitFrame(void)
{
    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pWaitDstStageMask = &stageFlags,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAcquiredSemaphores[imageAcquiredSemaphoreIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &frames[curFrameIndex].command.semaphore,
        .commandBufferCount = 1,
        .pCommandBuffers = &frames[curFrameIndex].command.commandBuffer,
    };

    vkWaitForFences(device, 1, &frames[curFrameIndex].command.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &frames[curFrameIndex].command.fence);

    V_ASSERT( vkQueueSubmit(graphicsQueues[0], 1, &si, frames[curFrameIndex].command.fence) );
}

void tanto_r_SubmitUI(const Tanto_V_Command cmd)
{
    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pWaitDstStageMask = &stageFlags,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frames[curFrameIndex].command.semaphore,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &presentationSemaphores[curFrameIndex],
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd.commandBuffer
    };

    V_ASSERT( vkQueueSubmit(graphicsQueues[0], 1, &si, cmd.fence) );
}

bool tanto_r_PresentFrame(void)
{
    VkResult r;
    const VkPresentInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentationSemaphores[curFrameIndex],
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

void tanto_r_RegisterSwapchainRecreationFn(Tanto_R_SwapchainRecreationFn fn)
{
    swapchainRecreationFns[swapRecreateFnCount] = fn;
    swapRecreateFnCount++;
    assert(swapRecreateFnCount < MAX_SWAP_RECREATE_FNS);
}

Tanto_R_Frame* tanto_r_GetFrame(const int8_t index)
{
    assert( index >= 0 );
    assert( index < TANTO_FRAME_COUNT );
    return &frames[index];
}

const uint32_t tanto_r_GetCurrentFrameIndex(void)
{
    return curFrameIndex;
}

void tanto_r_CleanUp(void)
{
    if (tanto_v_config.rayTraceEnabled)
        tanto_r_RayTraceCleanUp();
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        vkDestroySemaphore(device, imageAcquiredSemaphores[i], NULL);
        vkDestroySemaphore(device, presentationSemaphores[i], NULL);
        vkDestroyImageView(device, frames[i].swapImage.view, NULL);
        tanto_v_DestroyCommand(frames[i].command);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    swapRecreateFnCount = 0;
}

void tanto_r_WaitOnFrame(int8_t frameIndex)
{
    vkWaitForFences(device, 1, &frames[frameIndex].command.fence, VK_TRUE, UINT64_MAX);
}

VkFormat tanto_r_GetOffscreenColorFormat(void) { return offscreenColorFormat; }
VkFormat tanto_r_GetDepthFormat(void)          { return depthFormat; }
VkFormat tanto_r_GetSwapFormat(void)           { return swapFormat; }

void tanto_r_DrawScene(const VkCommandBuffer cmdBuf, const Tanto_S_Scene* scene)
{
    for (int i = 0; i < scene->primCount; i++) 
    {
        tanto_r_DrawPrim(cmdBuf, &scene->prims[i]);
    }
}
