#include "v_swapchain.h"
#include "common.h"
#include "dtags.h"
#include "hell/input.h"
#include "s_scene.h"
#include "v_command.h"
#include "v_private.h"
#include "v_video.h"
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
    VkImage                      images[SWAPCHAIN_IMAGE_COUNT];
    VkImageView                  views[SWAPCHAIN_IMAGE_COUNT];
    uint32_t                     imageCount;
    VkFormat                     format;
    VkSwapchainKHR               swapchain;
    VkImageUsageFlags            imageUsageFlags;
    // native windows are abstrated by surfaces. a swapchain can only be
    // associated with one window. 30.7 in spec.
    VkSurfaceKHR                 surface;
    uint32_t                     acquiredImageIndex;
    bool                         dirty;
    uint32_t                     width;
    uint32_t                     height;
} Obdn_Swapchain;

static VkExtent2D
getCorrectedSwapchainDimensions(const VkSurfaceCapabilitiesKHR capabilities,
                                VkExtent2D                     hint)
{
    VkExtent2D dim = {};
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
initSurfaceXcb(const Hell_Window* window, VkSurfaceKHR* surface)
{
    const XcbWindow*                w  = window->typeSpecificData;
    const VkXcbSurfaceCreateInfoKHR ci = {
        .sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = w->connection,
        .window     = w->window,
    };

    V_ASSERT(vkCreateXcbSurfaceKHR(*obdn_v_GetInstance(), &ci, NULL, surface));
}
#endif

#ifdef WINDOWS
static void
initSurfaceWin32(const Hell_Window* window, VkSurfaceKHR* surface)
{
    const Win32Window*          w  = window->typeSpecificData;
    VkWin32SurfaceCreateInfoKHR ci = {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = w->hinstance,
        .hwnd      = w->hwnd};

    V_ASSERT(
        vkCreateWin32SurfaceKHR(*obdn_v_GetInstance(), &ci, NULL, &surface));
}
#endif

static void
initSurface(const Hell_Window* window, VkSurfaceKHR* surface)
{
    assert(window);
    assert(window->typeSpecificData);
#ifdef UNIX
    initSurfaceXcb(window, surface);
#elif defined(WINDOWS)
    initSurfaceWin32(window, surface);
#endif
    obdn_Announce("Vulkan Surface initialized.\n");
}

static VkFormat
chooseFormat(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
{
    uint32_t formatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount,
                                         NULL);
    VkSurfaceFormatKHR surfaceFormats[formatsCount];
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

static void
createSwapchainWithSurface(const VkSurfaceKHR surface, const uint32_t widthHint,
                           const uint32_t heightHint, const VkFormat swapFormat,
                           const VkImageUsageFlags usageFlags, const VkFormat format,
                           uint32_t* correctedWidth, uint32_t* correctedHeight,
                           VkSwapchainKHR* swapchain)
{
    assert(surface);
    VkBool32               supported;
    const VkPhysicalDevice physicalDevice = obdn_v_GetPhysicalDevice();
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, surface,
                                         &supported);

    assert(supported == VK_TRUE);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount, NULL);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount, presentModes);

    // const VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // i
    // already know its supported
    const VkPresentModeKHR presentMode =
        VK_PRESENT_MODE_IMMEDIATE_KHR; // I get less input lag with this mode

    VkSurfaceCapabilitiesKHR capabilities;
    V_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                                       &capabilities));

    assert(capabilities.supportedUsageFlags &
           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    DPRINT("Surface Capabilities: Min swapchain image count: %d\n",
           capabilities.minImageCount);

    VkExtent2D swapDim = getCorrectedSwapchainDimensions(
        capabilities, (VkExtent2D){widthHint, heightHint});

    *correctedWidth = swapDim.width;
    *correctedHeight = swapDim.height;

    const VkSwapchainCreateInfoKHR ci = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = surface,
        .minImageCount         = 2,
        .imageFormat           = format,
        .imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent           = swapDim,
        // number of views in a multiview / stereo surface
        .imageArrayLayers      = 1,
        .imageUsage            = usageFlags,
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
        .presentMode           = presentMode,
        // allows pixels convered by another window to be
        // clipped. but will mess up saving the swap image.        .clipped =
        // VK_FALSE,

        .oldSwapchain = VK_NULL_HANDLE};

    V_ASSERT(vkCreateSwapchainKHR(device, &ci, NULL, swapchain));

    DPRINT("Swapchain created successfully.\n");
}

static void
createSwapchainImageViews(const VkSwapchainKHR swapchain, const VkFormat format,
                          const uint32_t width, const uint32_t height,
                          uint32_t* imageCount, VkImage* images,
                          VkImageView* views)
{
    V_ASSERT(vkGetSwapchainImagesKHR(device, swapchain, imageCount, NULL));
    assert(SWAPCHAIN_IMAGE_COUNT == *imageCount);
    V_ASSERT(vkGetSwapchainImagesKHR(device, swapchain, imageCount, images));

    for (int i = 0; i < *imageCount; i++)
    {
        VkImageSubresourceRange ssr = {.baseArrayLayer = 0,
                                       .layerCount     = 1,
                                       .baseMipLevel   = 0,
                                       .levelCount     = 1,
                                       .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};

        VkImageViewCreateInfo imageViewInfo = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .subresourceRange = ssr,
            .format           = format,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .image            = images[i]};

        V_ASSERT(vkCreateImageView(device, &imageViewInfo, NULL, &views[i]));

        VkExtent3D extent3d = {.width = width, .height = height, .depth = 1};
    }
    DPRINT("Swapchain image views created successfully.\n");
}

static void
recreateSwapchain(const VkSurfaceKHR surface, const uint32_t widthHint,
                  const uint32_t heightHint, const VkFormat swapFormat,
                  const VkImageUsageFlags usageFlags, 
                  uint32_t* correctedWidth, uint32_t* correctedHeight,
                  bool* dirtyFlag, VkSwapchainKHR* swapchain,
                  uint32_t* imageCount,
                  VkImage* images,
                  VkImageView* views)
{
    for (int i = 0; i < *imageCount; i++)
    {
        vkDestroyImageView(device, views[i], NULL);
    }
    vkDestroySwapchainKHR(device, *swapchain, NULL);
    createSwapchainWithSurface(surface, widthHint, heightHint, swapFormat,
                               usageFlags, swapFormat, correctedWidth, correctedHeight, swapchain);
    createSwapchainImageViews(*swapchain, swapFormat, *correctedWidth, *correctedHeight, imageCount, images, views);
    *dirtyFlag = true;
}

// this just sets the swapWidth and swapHeight and we rely on the call to
// request frame to detect that the swapchain needs to be recreated. we do this
// because we can get many resize events within a frame and we don't want to
// call recreate for every one of them. also, we may eventually run iterations
// of the main loop that dont render frames. this does mean that
// obdn_v_RequestFrame MUST be called before rendering on a given frame.
static Hell_I_ResizeData resizeEventData;
static bool              resizeEventOccurred;

static bool
onWindowResizeEvent(const Hell_I_Event* ev)
{
    resizeEventData     = ev->data.resizeData;
    resizeEventOccurred = true;
    return false;
}

Obdn_Swapchain* obdn_AllocSwapchain(void)
{
    return hell_Malloc(sizeof(Obdn_Swapchain));
}

void
obdn_InitSwapchain(Obdn_Swapchain*         swapchain,
                   const VkImageUsageFlags swapImageUsageFlags_,
                   const Hell_Window*      hellWindow)
{
    memset(swapchain, 0, sizeof(Obdn_Swapchain));
    swapchain->imageUsageFlags = swapImageUsageFlags_;
    initSurface(hellWindow, &swapchain->surface);
    swapchain->format = chooseFormat(obdn_v_GetPhysicalDevice(), swapchain->surface);
    createSwapchainWithSurface(
        swapchain->surface, hellWindow->width, hellWindow->height,
        SWAPCHAIN_DEFAULT_FORMAT, swapImageUsageFlags_, swapchain->format,
        &swapchain->width, &swapchain->height, &swapchain->swapchain);
    createSwapchainImageViews(swapchain->swapchain, swapchain->format,
                              swapchain->width, swapchain->height,
                              &swapchain->imageCount, swapchain->images,
                              swapchain->views);
    hell_i_Subscribe(onWindowResizeEvent, HELL_I_WINDOW_BIT);
    obdn_Announce("Swapchain initialized.\n");
}

void
obdn_ShutdownSwapchain(Obdn_Swapchain* swapchain)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
    {
        vkDestroyImageView(device, swapchain->views[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapchain->swapchain, NULL);
    vkDestroySurfaceKHR(*obdn_v_GetInstance(), swapchain->surface, NULL);
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

#define WAIT_TIME_NS 500000

unsigned
obdn_AcquireSwapchainImage(Obdn_Swapchain* swapchain, VkFence* fence,
                           VkSemaphore* semaphore, bool* dirty)
{
    VkResult r;
    *dirty = false;
retry:
    if (resizeEventOccurred)
    {
        vkDeviceWaitIdle(device);
        recreateSwapchain(
            swapchain->surface, resizeEventData.width, resizeEventData.height,
            swapchain->format, swapchain->imageUsageFlags,
            &swapchain->width, &swapchain->height, &swapchain->dirty,
            &swapchain->swapchain, &swapchain->imageCount, swapchain->images,
            swapchain->views);
        resizeEventOccurred = false;
        *dirty = true;
    }
    r = vkAcquireNextImageKHR(device, swapchain->swapchain, WAIT_TIME_NS,
                              *semaphore, *fence,
                              &swapchain->acquiredImageIndex);
    if (VK_ERROR_OUT_OF_DATE_KHR == r)
    {
        vkDeviceWaitIdle(device);
        recreateSwapchain(
            swapchain->surface, swapchain->width, swapchain->height,
            swapchain->format, swapchain->imageUsageFlags,
            &swapchain->width, &swapchain->height, &swapchain->dirty,
            &swapchain->swapchain, &swapchain->imageCount, swapchain->images,
            swapchain->views);
        *dirty = true;
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
    return swapchain->acquiredImageIndex;
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
    presentResult = vkQueuePresentKHR(obdn_v_GetPresentQueue(), &info);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return false;
    }
    assert(VK_SUCCESS == presentResult);
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
    return swapchain->views[index];
}

size_t
obdn_SizeOfSwapchain(void)
{
    return sizeof(Obdn_Swapchain);
}
