#include "v_swapchain.h"
#include <hell/display.h>
#include <hell/platform.h>
#include <string.h>
#include "hell/input.h"
#include "private.h"
#include "s_scene.h"
#include "t_def.h"
#include "v_video.h"
#include "v_private.h"
#ifdef UNIX
#include <hell/xcb_window_type.h>
#elif defined(WINDOWS)
#include <hell/win32_window.h>
#endif

static VkFormat swapFormat = VK_FORMAT_B8G8R8A8_SRGB;

static Obdn_R_Frame        frames[OBDN_FRAME_COUNT];
static uint32_t            curFrameIndex;

static VkSwapchainKHR      swapchain;

static uint32_t            swapWidth;
static uint32_t            swapHeight;

static VkSemaphore  imageAcquiredSemaphores[OBDN_FRAME_COUNT];
static uint8_t      imageAcquiredSemaphoreIndex = 0;
static uint64_t     frameCounter;
static bool         swapchainDirty;

#define MAX_SWAP_RECREATE_FNS 8

static uint8_t swapRecreateFnCount = 0;
static Obdn_R_SwapchainRecreationFn swapchainRecreationFns[MAX_SWAP_RECREATE_FNS];

static VkImageUsageFlags  swapImageUsageFlags;
static VkSurfaceKHR       surface;

static bool useOffscreenSwapchain = false;

static void correctSwapDimensions(const VkSurfaceCapabilitiesKHR capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        swapWidth  = capabilities.currentExtent.width;
        swapHeight = capabilities.currentExtent.height;
    } 
    else 
    {
        swapWidth  = MAX(capabilities.minImageExtent.width,  MIN(capabilities.maxImageExtent.width, swapWidth)); 
        swapHeight = MAX(capabilities.minImageExtent.width,  MIN(capabilities.maxImageExtent.width, swapHeight)); 
    }
}

#ifdef UNIX
static void initSurfaceXcb(const Hell_Window* window) 
{
    const XcbWindow* w = window->typeSpecificData;
    const VkXcbSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = w->connection,
        .window = w->window,
    };

    V_ASSERT( vkCreateXcbSurfaceKHR(*obdn_v_GetInstance(), &ci, NULL, &surface) );
    V1_PRINT("Surface created successfully.\n");
}
#endif

#ifdef WINDOWS
static void initSurfaceWin32(const Hell_Window* window) 
{
    const Win32Window* w = window->typeSpecificData;
    VkWin32SurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = w->hinstance,
        .hwnd = w->hwnd
    };

    V_ASSERT( vkCreateWin32SurfaceKHR(*obdn_v_GetInstance(), &ci, NULL, &surface) );
    V1_PRINT("Surface created successfully.\n");
}
#endif

static void initSurface(const Hell_Window* window)
{
    assert(window);
    assert(window->typeSpecificData);
    #ifdef UNIX
    initSurfaceXcb(window);
    #elif defined(WINDOWS)
    initSurfaceWin32(window);
    #endif
}

static void initSwapchainWithSurface()
{
    assert(surface);
    VkBool32 supported;
    const VkPhysicalDevice physicalDevice = obdn_v_GetPhysicalDevice();
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

    correctSwapDimensions(capabilities);

    VkExtent2D extent2D = {
        .width = swapWidth,
        .height = swapHeight
    };

    const VkSwapchainCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = swapFormat, //50
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent2D,
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

        VkExtent3D extent3d = {
            .width  = swapWidth,
            .height = swapHeight,
            .depth = 1
        };

        frames[i].extent = extent3d;
        frames[i].size =   extent3d.height * extent3d.width * 4; // TODO make this robust
    }

    V1_PRINT("Swapchain created successfully.\n");
}

static void initSwapchainOffscreen(void)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
        frames[i] = obdn_v_CreateImage(
                swapWidth, swapHeight, swapFormat,
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

static void recreateSwapchain(void)
{
    vkDeviceWaitIdle(device);
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
        for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
        {
            vkDestroyImageView(device, frames[i].view, NULL);   
        }
        vkDestroySwapchainKHR(device, swapchain, NULL);
        initSwapchainWithSurface();
    }
    for (int i = 0; i < swapRecreateFnCount; i++) 
    {
        swapchainRecreationFns[i]();   
    }
    swapchainDirty = true;
}

static uint32_t requestSwapchainFrame(Obdn_Mask* dirtyBits, Obdn_S_Window window)
{
    imageAcquiredSemaphoreIndex = frameCounter % OBDN_FRAME_COUNT;
    VkResult r;
retry:
    r = vkAcquireNextImageKHR(device, 
            swapchain, 
            1000000, 
            imageAcquiredSemaphores[imageAcquiredSemaphoreIndex], 
            VK_NULL_HANDLE, 
            &curFrameIndex);
    if (VK_ERROR_OUT_OF_DATE_KHR == r || swapchainDirty) 
    {
        recreateSwapchain();
        window[0] = swapWidth;
        window[1] = swapHeight;
        *dirtyBits |= OBDN_S_WINDOW_BIT;
        swapchainDirty = false;
        goto retry;
    }
    frameCounter++;
    return curFrameIndex;
}

static uint32_t requestOffscreenFrame(Obdn_Mask* dirtyBits, Obdn_S_Window window)
{
    curFrameIndex = frameCounter % OBDN_FRAME_COUNT;
    if (swapchainDirty)
    {
        recreateSwapchain();
        window[0] = swapWidth;
        window[1] = swapHeight;
        *dirtyBits |= OBDN_S_WINDOW_BIT;
        swapchainDirty = false;
    }
    frameCounter++;
    return curFrameIndex;
}

_Static_assert(sizeof(Obdn_S_Window) == sizeof(uint16_t) * 2, "Obdn_S_Window must be a size 2 array of uint16_t");

// must be called before making a call to render within the same frame.
const uint32_t obdn_v_RequestFrame(Obdn_Mask* dirtyFlag, Obdn_S_Window window)
{
    if (!useOffscreenSwapchain)
        return requestSwapchainFrame(dirtyFlag, window);
    else
        return requestOffscreenFrame(dirtyFlag, window);
}

// this just sets the swapWidth and swapHeight and we rely on the call to request frame to detect that
// the swapchain needs to be recreated. we do this because we can get many resize events within a frame
// and we don't want to call recreate for every one of them. also, we may eventually run iterations of 
// the main loop that dont render frames. 
// this does mean that obdn_v_RequestFrame MUST be called before rendering on a given frame.
static bool onWindowResizeEvent(const Hell_I_Event* ev)
{
    swapWidth  = ev->data.resizeData.width;
    swapHeight = ev->data.resizeData.height;
    swapchainDirty = true;
    //recreateSwapchain();
    return false;
}

void obdn_v_InitSwapchain(const VkImageUsageFlags swapImageUsageFlags_, const Hell_Window* hellWindow)
{
    curFrameIndex = 0;
    frameCounter  = 0;
    swapRecreateFnCount = 0;
    imageAcquiredSemaphoreIndex = 0;
    swapImageUsageFlags = swapImageUsageFlags_;
    useOffscreenSwapchain = !hellWindow;
    if (!hellWindow)
    {
        swapFormat = VK_FORMAT_R8G8B8A8_UNORM;
        swapWidth  = 666; // arbitrary defaults
        swapHeight = 666;
        initSwapchainOffscreen();
    }
    else
    {
        initSurface(hellWindow);
        initSwapchainWithSurface();
    }
    initSwapchainSemaphores();
    hell_i_Subscribe(onWindowResizeEvent, HELL_I_WINDOW_BIT);
    printf("Obdn Renderer initialized.\n");
}

bool obdn_v_PresentFrame(VkSemaphore waitSemaphore)
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

void obdn_v_RegisterSwapchainRecreationFn(Obdn_R_SwapchainRecreationFn fn)
{
    swapchainRecreationFns[swapRecreateFnCount] = fn;
    swapRecreateFnCount++;
    assert(swapRecreateFnCount < MAX_SWAP_RECREATE_FNS);
}

void obdn_v_UnregisterSwapchainRecreateFn(Obdn_R_SwapchainRecreationFn fn)
{
    assert(swapRecreateFnCount > 0);
    printf("R) Unregistering SCR fn...\n");
    int fnIndex = -1;
    for (int i = 0; i < swapRecreateFnCount ; i++)
    {
        if (swapchainRecreationFns[i] == fn)
        {
            fnIndex = i;
            break;
        }
    }
    if (fnIndex != -1)
        memmove(swapchainRecreationFns + fnIndex, 
                swapchainRecreationFns + fnIndex + 1, 
                (--swapRecreateFnCount - fnIndex) * sizeof(*swapchainRecreationFns)); // should only decrement the count if fnIndex is 0
}

const Obdn_R_Frame* obdn_v_GetFrame(const int8_t index)
{
    assert( index >= 0 );
    assert( index < OBDN_FRAME_COUNT );
    return &frames[index];
}

const uint32_t obdn_v_GetCurrentFrameIndex(void)
{
    return curFrameIndex;
}

void obdn_v_CleanUpSwapchain(void)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        vkDestroySemaphore(device, imageAcquiredSemaphores[i], NULL);
        if (useOffscreenSwapchain)
            obdn_v_FreeImage(&frames[i]);
        else
            vkDestroyImageView(device, frames[i].view, NULL);
    }
    if (!useOffscreenSwapchain)
    {
        vkDestroySwapchainKHR(device, swapchain, NULL);
        vkDestroySurfaceKHR(*obdn_v_GetInstance(), surface, NULL);
    }
    swapRecreateFnCount = 0;
    printf("Obsidian render cleaned up.\n");
}

VkFormat obdn_v_GetSwapFormat(void) { return swapFormat; }

VkExtent2D          obdn_v_GetSwapExtent(void)
{
    VkExtent2D ex = {
        .width = swapWidth,
        .height = swapHeight
    };
    return ex;
}
