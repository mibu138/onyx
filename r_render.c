#include "t_def.h"
#include "r_render.h"
#include "r_geo.h"
#include "v_command.h"
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
VkFormat swapFormat                 = VK_FORMAT_B8G8R8A8_SRGB;

static Obdn_R_Frame        frames[OBDN_FRAME_COUNT];
static uint32_t            curFrameIndex;

static VkSwapchainKHR      swapchain;

static VkSemaphore  imageAcquiredSemaphores[OBDN_FRAME_COUNT];
static uint8_t      imageAcquiredSemaphoreIndex = 0;
static uint64_t            frameCounter;

#define MAX_SWAP_RECREATE_FNS 8
static uint8_t swapRecreateFnCount = 0;
static Obdn_R_SwapchainRecreationFn swapchainRecreationFns[MAX_SWAP_RECREATE_FNS];

static VkImageUsageFlags swapImageUsageFlags;

static bool useOffscreenSwapchain = false;

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {

        VkExtent2D actualExtent = {
            OBDN_WINDOW_WIDTH,
            OBDN_WINDOW_HEIGHT
        };

        actualExtent.width =  MAX(capabilities.minImageExtent.width,  MIN(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = MAX(capabilities.minImageExtent.height, MIN(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

static void initSwapchainWithSurface()
{
    VkBool32 supported;
    const VkSurfaceKHR surface = obdn_v_GetSurface();
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, surface, &supported);

    assert(supported == VK_TRUE);

    VkSurfaceCapabilitiesKHR capabilities;
    V_ASSERT( vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities) );

    uint32_t formatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, NULL);
    VkSurfaceFormatKHR surfaceFormats[formatsCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, surfaceFormats);

    V1_PRINT("Surface formats: \n");
    for (int i = 0; i < formatsCount; i++) {
        V1_PRINT("Format: %d   Colorspace: %d\n", surfaceFormats[i].format, surfaceFormats[i].colorSpace);
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

    //const VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // i already know its supported 
    const VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // I get less input lag with this mode

    assert(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    V1_PRINT("Surface Capabilities: Min swapchain image count: %d\n", capabilities.minImageCount);

    const VkExtent2D extent = chooseSwapExtent(capabilities);
    OBDN_WINDOW_WIDTH =  extent.width;
    OBDN_WINDOW_HEIGHT = extent.height;

    const VkSwapchainCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = swapFormat, //50
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1, // number of views in a multiview / stereo surface
        .imageUsage = swapImageUsageFlags,
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
    assert(OBDN_FRAME_COUNT == imageCount);
    VkImage swapchainImages[imageCount];
    V_ASSERT( vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages) );

    for (int i = 0; i < imageCount; i++) 
    {
        frames[i].handle = swapchainImages[i];
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
            .image = frames[i].handle,
        };

        V_ASSERT( vkCreateImageView(device, &imageViewInfo, NULL, &frames[i].view) );

        VkExtent3D extent = {
            .width = OBDN_WINDOW_WIDTH,
            .height = OBDN_WINDOW_HEIGHT,
            .depth = 1
        };

        frames[i].extent = extent;
        frames[i].size =   extent.height * extent.width * 4; // TODO make this robust
    }

    V1_PRINT("Swapchain created successfully.\n");
}

static void initSwapchainOffscreen()
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
        frames[i] = obdn_v_CreateImage(
                OBDN_WINDOW_WIDTH, OBDN_WINDOW_HEIGHT, swapFormat,
                swapImageUsageFlags, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT,
                1, OBDN_V_MEMORY_EXTERNAL_DEVICE_TYPE);
}

static void initSwapchainSemaphores(void)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        VkSemaphoreCreateInfo semaCi = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        V_ASSERT( vkCreateSemaphore(device, &semaCi, NULL, &imageAcquiredSemaphores[i]) );
        printf("Created swapchain semaphore: %p \n", imageAcquiredSemaphores[i]);
    }
}

static uint32_t requestSwapchainFrame(void)
{
    imageAcquiredSemaphoreIndex = frameCounter % OBDN_FRAME_COUNT;
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
        obdn_r_RecreateSwapchain();
        goto retry;
    }
    frameCounter++;
    return curFrameIndex;
}

static uint32_t requestOffscreenFrame(void)
{
    curFrameIndex = frameCounter % OBDN_FRAME_COUNT;
    frameCounter++;
    return curFrameIndex;
}

const uint32_t obdn_r_RequestFrame(void)
{
    if (!useOffscreenSwapchain)
        return requestSwapchainFrame();
    else
        return requestOffscreenFrame();
}

void obdn_r_Init(const VkImageUsageFlags swapImageUsageFlags_, bool offscreenSwapchain)
{
    curFrameIndex = 0;
    frameCounter  = 0;
    swapRecreateFnCount = 0;
    imageAcquiredSemaphoreIndex = 0;
    swapImageUsageFlags = swapImageUsageFlags_;
    useOffscreenSwapchain = offscreenSwapchain;
    if (offscreenSwapchain)
    {
        swapFormat = VK_FORMAT_R8G8B8A8_UNORM;
        initSwapchainOffscreen();
    }
    else
    {
        initSwapchainWithSurface();
    }
    initSwapchainSemaphores();
    printf("Obdn Renderer initialized.\n");
}

void obdn_r_RecreateSwapchain(void)
{
    vkDeviceWaitIdle(device);
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        vkDestroyImageView(device, frames[i].view, NULL);   
    }
    if (useOffscreenSwapchain)
    {
        for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
        {
            obdn_v_FreeImage(&frames[i]);
        }
        initSwapchainOffscreen();
    }
    else
    {
        vkDestroySwapchainKHR(device, swapchain, NULL);
        initSwapchainWithSurface();
    }
    for (int i = 0; i < swapRecreateFnCount; i++) 
    {
        swapchainRecreationFns[i]();   
    }
}

bool obdn_r_PresentFrame(VkSemaphore waitSemaphore)
{
    VkResult r;
    VkSemaphore waitSemaphores[2] = {waitSemaphore, imageAcquiredSemaphores[imageAcquiredSemaphoreIndex]};
    const VkPresentInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .waitSemaphoreCount = 2,
        .pWaitSemaphores = waitSemaphores,
        .pResults = &r,
        .pImageIndices = &curFrameIndex,
    };

    VkResult presentResult;
    presentResult = vkQueuePresentKHR(obdn_v_GetPresentQueue(), &info);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        return false;
    assert( VK_SUCCESS == r );
    return true;
}

void obdn_r_RegisterSwapchainRecreationFn(Obdn_R_SwapchainRecreationFn fn)
{
    swapchainRecreationFns[swapRecreateFnCount] = fn;
    swapRecreateFnCount++;
    assert(swapRecreateFnCount < MAX_SWAP_RECREATE_FNS);
}

const Obdn_R_Frame* obdn_r_GetFrame(const int8_t index)
{
    assert( index >= 0 );
    assert( index < OBDN_FRAME_COUNT );
    return &frames[index];
}

const uint32_t obdn_r_GetCurrentFrameIndex(void)
{
    return curFrameIndex;
}

void obdn_r_CleanUp(void)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        vkDestroySemaphore(device, imageAcquiredSemaphores[i], NULL);
        vkDestroyImageView(device, frames[i].view, NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    swapRecreateFnCount = 0;
}

VkFormat obdn_r_GetOffscreenColorFormat(void) { return offscreenColorFormat; }
VkFormat obdn_r_GetDepthFormat(void)          { return depthFormat; }
VkFormat obdn_r_GetSwapFormat(void)           { return swapFormat; }

void obdn_r_DrawScene(const VkCommandBuffer cmdBuf, const Obdn_S_Scene* scene)
{
    for (int i = 0; i < scene->primCount; i++) 
    {
        obdn_r_DrawPrim(cmdBuf, &scene->prims[i].rprim);
    }
}
