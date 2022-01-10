#include "swapchain.h"
#include "common.h"
#include "dtags.h"
#include "hell/input.h"
#include "scene.h"
#include "command.h"
#include "private.h"
#include "video.h"
#include <hell/common.h>
#include <hell/debug.h>
#include <hell/minmax.h>
#include <hell/platform.h>
#include <hell/window.h>
#include <string.h>
#ifdef UNIX
#include <hell/xcb_window_type.h>
#elif defined(WINDOWS)
#include <hell/win32_window.h>
#endif

#define DPRINT(fmt, ...)                                                       \
    hell_DebugPrint(OBDN_DEBUG_TAG_SWAP, fmt, ##__VA_ARGS__)

#define SWAPCHAIN_IMAGE_COUNT 2
#define MAX_SWAP_RECREATE_FNS 8

#define SWAPCHAIN_DEFAULT_FORMAT VK_FORMAT_B8G8R8A8_SRGB

typedef struct Obdn_Swapchain {
    VkDevice                     device;
    VkQueue                      presentQueue;
    Obdn_Frame             framebuffers[SWAPCHAIN_IMAGE_COUNT];
    uint32_t                     aovCount;
    Obdn_AovInfo                 aovInfos[OBDN_MAX_AOVS];
    uint32_t                     imageCount;
    VkFormat                     format;
    VkSwapchainKHR               swapchain;
    VkImageUsageFlags            imageUsageFlags;
    // native windows are abstrated by surfaces. a swapchain can only be
    // associated with one window. 30.7 in spec.
    VkSurfaceKHR                 surface;
    uint32_t                     acquiredImageIndex;
    bool                         hasDepthImages;
    bool                         dirty;
    uint32_t                     width;
    uint32_t                     height;
    VkPhysicalDevice             physicalDevice;
    VkPresentModeKHR             presentMode;
    Obdn_Memory*                 memory;
} Obdn_Swapchain;

static VkExtent2D
getCorrectedSwapchainDimensions(const VkSurfaceCapabilitiesKHR capabilities,
                                VkExtent2D                     hint)
{
    VkExtent2D dim = {0};
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        dim.width  = capabilities.currentExtent.width;
        dim.height = capabilities.currentExtent.height;
    }
    else
    {
        dim.width  = MAX(capabilities.minImageExtent.width,
                        MIN(capabilities.maxImageExtent.width, hint.width));
        dim.height = MAX(capabilities.minImageExtent.width,
                         MIN(capabilities.maxImageExtent.width, hint.height));
    }
    return dim;
}

#ifdef UNIX
static void
initSurfaceXcb(const VkInstance instance, const Hell_Window* window, VkSurfaceKHR* surface)
{
    const VkXcbSurfaceCreateInfoKHR ci = {
        .sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = (xcb_connection_t*)hell_GetXcbConnection(window),
        .window     = *(xcb_window_t*)hell_GetXcbWindowPtr(window)
    };

    V_ASSERT(vkCreateXcbSurfaceKHR(instance, &ci, NULL, surface));
}
#endif

#ifdef WIN32
static void
initSurfaceWin32(const VkInstance instance, const Hell_Window* window, VkSurfaceKHR* surface)
{
    HWND hwnd = *(HWND*)hell_GetHwndPtr(window);
    HINSTANCE hinstance = *(HINSTANCE*)hell_GetHinstancePtr(window);
    VkWin32SurfaceCreateInfoKHR ci = {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = hinstance,
        .hwnd      = hwnd
    };

    V_ASSERT(vkCreateWin32SurfaceKHR(instance, &ci, NULL, surface));
}
#endif

static void
initSurface(const VkInstance instance, const Hell_Window* window, VkSurfaceKHR* surface)
{
    assert(window);
#ifdef UNIX
    initSurfaceXcb(instance, window, surface);
#elif defined(WIN32)
    initSurfaceWin32(instance, window, surface);
#endif
    obdn_Announce("Vulkan Surface initialized.\n");
}

static VkFormat
chooseFormat(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
{
    uint32_t formatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount,
                                         NULL);
    VkSurfaceFormatKHR* surfaceFormats = hell_Malloc(sizeof(*surfaceFormats) * formatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount,
                                         surfaceFormats);

    DPRINT("Surface formats: \n");
    for (int i = 0; i < formatsCount; i++)
    {
        DPRINT("Format: %d   Colorspace: %d\n", surfaceFormats[i].format,
               surfaceFormats[i].colorSpace);
    }

    return SWAPCHAIN_DEFAULT_FORMAT; // TODO: actually choose a format based on
                                     // physical device;
}

static void initSwapchainPresentMode(const VkPhysicalDevice physicalDevice,
        const VkSurfaceKHR surface,
        Obdn_Swapchain* swapchain)
{
    assert(surface);
    VkBool32               supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, surface,
                                         &supported);

    assert(supported == VK_TRUE);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount, NULL);
    assert(presentModeCount < 10); 
    VkPresentModeKHR presentModes[10];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount, presentModes);

    // const VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // i
    // already know its supported
    swapchain->presentMode =
        VK_PRESENT_MODE_IMMEDIATE_KHR; // I get less input lag with this mode

}

static void
createSwapchainWithSurface(Obdn_Swapchain* swapchain,
                           const uint32_t widthHint,
                           const uint32_t heightHint)
{
    VkSurfaceCapabilitiesKHR capabilities;
    V_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(swapchain->physicalDevice, swapchain->surface,
                                                       &capabilities));

    assert(capabilities.supportedUsageFlags &
           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    DPRINT("Surface Capabilities: Min swapchain image count: %d\n",
           capabilities.minImageCount);

    VkExtent2D swapDim = getCorrectedSwapchainDimensions(
        capabilities, (VkExtent2D){widthHint, heightHint});

    swapchain->width  = swapDim.width;
    swapchain->height = swapDim.height;
    swapchain->imageCount = capabilities.minImageCount;

    assert(swapchain->imageCount == 2); // for now

    const VkSwapchainCreateInfoKHR ci = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = swapchain->surface,
        .minImageCount         = swapchain->imageCount,
        .imageFormat           = swapchain->format,
        .imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent           = swapDim,
        // number of views in a multiview / stereo surface
        .imageArrayLayers      = 1,
        .imageUsage            = swapchain->imageUsageFlags,
        // queue sharing. see
        // vkspec section 11.7.
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        // dont need with exclusive sharing
        .queueFamilyIndexCount = 0,
        // ditto
        .pQueueFamilyIndices   = NULL,
        .preTransform          = capabilities.currentTransform,
        // dunno. may affect blending
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = swapchain->presentMode,
        // allows pixels convered by another window to be
        // clipped. but will mess up saving the swap image.        .clipped =
        // VK_FALSE,

        .oldSwapchain = VK_NULL_HANDLE};

    V_ASSERT(vkCreateSwapchainKHR(swapchain->device, &ci, NULL, &swapchain->swapchain));

    DPRINT("Swapchain created successfully.\n");
}

static void 
createSwapchainOffscreen(Obdn_Swapchain* swapchain,
        const uint32_t widthHint, const uint32_t heightHint)
{
    swapchain->width = widthHint;
    swapchain->height = heightHint;
}

#define MAX_SWAPCHAIN_IMAGES 8

static void
createSwapchainFramebuffers(Obdn_Swapchain* swapchain, uint32_t aovCount, Obdn_AovInfo aovInfos[/*aovCount*/])
{
    VkImage imageBuffer[MAX_SWAPCHAIN_IMAGES];
    V_ASSERT(vkGetSwapchainImagesKHR(swapchain->device, swapchain->swapchain, &swapchain->imageCount, NULL));
    assert(SWAPCHAIN_IMAGE_COUNT == swapchain->imageCount);
    V_ASSERT(vkGetSwapchainImagesKHR(swapchain->device, swapchain->swapchain, &swapchain->imageCount, imageBuffer));

    assert(aovCount > 0);
    aovInfos[0].format = swapchain->format;
    aovInfos[0].aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    for (int i = 0; i < swapchain->imageCount; i++)
    {
        Obdn_Frame* fb = &swapchain->framebuffers[i];
        memset(fb, 0, sizeof(Obdn_Frame));
        Obdn_Image* colorImage = &fb->aovs[0];
        colorImage->handle = imageBuffer[i];

        VkImageSubresourceRange ssr = {.baseArrayLayer = 0,
                                       .layerCount     = 1,
                                       .baseMipLevel   = 0,
                                       .levelCount     = 1,
                                       .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};

        VkImageViewCreateInfo imageViewInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .subresourceRange = ssr,
            .format           = swapchain->format,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .image            = colorImage->handle};

        V_ASSERT(vkCreateImageView(swapchain->device, &imageViewInfo, NULL, &colorImage->view));

        colorImage->aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImage->mipLevels = 1;
        colorImage->extent.width = swapchain->width;
        colorImage->extent.height = swapchain->height;
        colorImage->extent.depth = 1;
        colorImage->format = swapchain->format;
        colorImage->usageFlags = swapchain->imageUsageFlags;

        for (uint32_t i = 1; i < aovCount; i++)
        {
            fb->aovs[i] = obdn_CreateImage(
                swapchain->memory, swapchain->width, swapchain->height,
                aovInfos[i].format, aovInfos[i].usageFlags,
                aovInfos[i].aspectFlags, VK_SAMPLE_COUNT_1_BIT, 1,
                OBDN_MEMORY_DEVICE_TYPE);
        }

        fb->index = i;
        fb->aovCount = aovCount;
        fb->dirty = true;
        fb->width = swapchain->width;
        fb->height = swapchain->height;
    }
    DPRINT("Swapchain framebuffers created successfully.\n");
}

static void 
recreateSwapchain(Obdn_Swapchain* swapchain, const uint32_t widthHint,
                  const uint32_t heightHint)
{
    for (int i = 0; i < swapchain->imageCount; i++)
    {
        Obdn_Frame* fb = &swapchain->framebuffers[i];
        vkDestroyImageView(swapchain->device, fb->aovs[0].view, NULL);
        for (uint32_t i = 1; i < fb->aovCount; i++)
        {
            obdn_FreeImage(&fb->aovs[i]);
        }
    }
    vkDestroySwapchainKHR(swapchain->device, swapchain->swapchain, NULL);
    createSwapchainWithSurface(
        swapchain, widthHint, heightHint);
    createSwapchainFramebuffers(swapchain, swapchain->aovCount, swapchain->aovInfos);
}

static bool
onWindowResizeEvent(const Hell_Event* ev, void* swapchainPtr)
{
    Obdn_Swapchain* swapchain = (Obdn_Swapchain*)swapchainPtr;
    swapchain->dirty = true;
    swapchain->width = hell_GetWindowResizeWidth(ev);
    swapchain->height = hell_GetWindowResizeHeight(ev);
    return false;
}

Obdn_Swapchain* obdn_AllocSwapchain(void)
{
    return hell_Malloc(sizeof(Obdn_Swapchain));
}

void
obdn_CreateSwapchain(const Obdn_Instance*  instance,
                   Obdn_Memory*            memory,
                   Hell_EventQueue*        eventQueue,
                   const Hell_Window*      hellWindow,
                   VkImageUsageFlags       usageFlags,
                   uint32_t                extraAovCount,
                   Obdn_AovInfo            extraAovInfos[/*extraAovCount*/],
                   Obdn_Swapchain*         swapchain)
{
    memset(swapchain, 0, sizeof(Obdn_Swapchain));
    uint32_t aovCount = extraAovCount + 1;
    assert(aovCount > 0);
    assert(aovCount <= OBDN_MAX_AOVS);
    Obdn_AovInfo aovInfos[OBDN_MAX_AOVS];
    memset(aovInfos, 0, sizeof(aovInfos));
    memcpy(&aovInfos[1], extraAovInfos, extraAovCount * sizeof(Obdn_AovInfo));
    aovInfos[0].usageFlags = usageFlags;
    aovInfos[0].aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    swapchain->device = instance->device;
    swapchain->memory = memory;
    swapchain->presentQueue = instance->presentQueue;
    swapchain->imageUsageFlags = usageFlags;
    swapchain->physicalDevice = instance->physicalDevice;
    swapchain->aovCount = aovCount;
    memcpy(swapchain->aovInfos, aovInfos, sizeof(Obdn_AovInfo) * aovCount);
    initSurface(instance->vkinstance, hellWindow, &swapchain->surface);
    swapchain->format = chooseFormat(instance->physicalDevice, swapchain->surface);
    aovInfos[0].format = swapchain->format;
    initSwapchainPresentMode(instance->physicalDevice, swapchain->surface, swapchain);
    createSwapchainWithSurface(swapchain, hell_GetWindowWidth(hellWindow), hell_GetWindowHeight(hellWindow));
    createSwapchainFramebuffers(swapchain, aovCount, aovInfos);
    hell_Subscribe(eventQueue, HELL_EVENT_MASK_WINDOW_BIT, hell_GetWindowID(hellWindow), onWindowResizeEvent, swapchain);
    obdn_Announce("Swapchain initialized.\n");
}

void
obdn_DestroySwapchain(const Obdn_Instance* instance, Obdn_Swapchain* swapchain)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
    {
        Obdn_Frame* fb = &swapchain->framebuffers[i];
        vkDestroyImageView(swapchain->device, fb->aovs[0].view, NULL);
        for (uint32_t i = 1; i < fb->aovCount; i++)
        {
            obdn_FreeImage(&fb->aovs[i]);
        }
    }
    vkDestroySwapchainKHR(swapchain->device, swapchain->swapchain, NULL);
    vkDestroySurfaceKHR(instance->vkinstance, swapchain->surface, NULL);
    memset(swapchain, 0, sizeof(*swapchain));
    obdn_Announce("Swapchain shutdown.\n");
}

VkFormat
obdn_GetSwapchainFormat(const Obdn_Swapchain* swapchain)
{
    return swapchain->format;
}

VkExtent2D
obdn_GetSwapchainExtent(const Obdn_Swapchain* swapchain)
{
    VkExtent2D ex = {.width = swapchain->width, .height = swapchain->height};
    return ex;
}

VkExtent3D      obdn_GetSwapchainExtent3D(const Obdn_Swapchain* swapchain)
{
    VkExtent3D ex = {swapchain->width, swapchain->height, 1};
    return ex;
}

#define WAIT_TIME_NS 500000

const Obdn_Frame*
obdn_AcquireSwapchainFrame(Obdn_Swapchain* swapchain, VkFence fence,
                           VkSemaphore semaphore)
{
    assert(semaphore);
    VkResult r;
retry:
    if (swapchain->dirty)
    {
        vkDeviceWaitIdle(swapchain->device);
        recreateSwapchain(swapchain,
            swapchain->width, swapchain->height);
        swapchain->dirty = false;
    }
    r = vkAcquireNextImageKHR(swapchain->device, swapchain->swapchain, WAIT_TIME_NS,
                              semaphore, fence,
                              &swapchain->acquiredImageIndex);
    if (VK_ERROR_OUT_OF_DATE_KHR == r)
    {
        vkDeviceWaitIdle(swapchain->device);
        recreateSwapchain(swapchain, swapchain->width, swapchain->height);
        swapchain->dirty = false;
        goto retry;
    }
    if (VK_SUBOPTIMAL_KHR == r)
    {
        DPRINT("Suboptimal swapchain\n");
    }
    if (VK_ERROR_DEVICE_LOST == r)
    {
        hell_Error(HELL_ERR_FATAL, "Device lost\n");
    }
    return &swapchain->framebuffers[swapchain->acquiredImageIndex];
}

bool
obdn_PresentFrame(Obdn_Swapchain* swapchain, const uint32_t semaphoreCount,
                  VkSemaphore* waitSemaphores)
{
    const VkPresentInfoKHR info = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                   .swapchainCount     = 1,
                                   .pSwapchains        = &swapchain->swapchain,
                                   .waitSemaphoreCount = semaphoreCount,
                                   .pWaitSemaphores    = waitSemaphores,
                                   // pResults is for per swapchain results
                                   // we only use one so can put NULL
                                   .pResults           = NULL,
                                   .pImageIndices =
                                       &swapchain->acquiredImageIndex};

    VkResult presentResult;
    presentResult = vkQueuePresentKHR(swapchain->presentQueue, &info);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return false;
    }
    assert(VK_SUCCESS == presentResult);
    swapchain->framebuffers[swapchain->acquiredImageIndex].dirty = false;
    return true;
}

void
obdn_FreeSwapchain(Obdn_Swapchain* swapchain)
{
    hell_Free(swapchain);
}

unsigned
obdn_GetSwapchainWidth(const Obdn_Swapchain* swapchain)
{
    return swapchain->width;
}

unsigned
obdn_GetSwapchainHeight(const Obdn_Swapchain* swapchain)
{
    return swapchain->height;
}

VkImageView
obdn_GetSwapchainImageView(const Obdn_Swapchain* swapchain, int index)
{
    return swapchain->framebuffers[index].aovs[0].view;
}

size_t
obdn_SizeOfSwapchain(void)
{
    return sizeof(Obdn_Swapchain);
}

unsigned obdn_GetSwapchainImageCount(const Obdn_Swapchain* swapchain)
{
    return swapchain->imageCount;
}

//const VkImageView* obdn_GetSwapchainImageViews(const Obdn_Swapchain* swapchain)
//{
//    return swapchain->colorImageViews;
//}
//
//VkImage obdn_GetSwapchainImage(const Obdn_Swapchain* swapchain, uint32_t index)
//{
//    assert(index < swapchain->imageCount);
//    return swapchain->colorImages[index];
//}

VkDeviceSize obdn_GetSwapchainImageSize(const Obdn_Swapchain *swapchain)
{
    return swapchain->width * swapchain->height * 4;
}

uint32_t obdn_GetSwapchainPixelByteCount(const Obdn_Swapchain* swapchain)
{
    return 4;
}

uint32_t obdn_GetSwapchainFrameCount(const Obdn_Swapchain* s)
{
    return s->imageCount;
}

const Obdn_Frame* obdn_GetSwapchainFrames(const Obdn_Swapchain* s)
{
    return s->framebuffers;
}
