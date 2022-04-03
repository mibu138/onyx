#ifndef V_SWAPCHAIN_H
#define V_SWAPCHAIN_H

#include "def.h"
#include "memory.h"
#include "command.h"
#include "frame.h"
#include "types.h"
#include <hell/window.h>

typedef void (*Onyx_R_SwapchainRecreationFn)(void);
typedef struct Onyx_Swapchain Onyx_Swapchain;

#define SWAPCHAIN_IMAGE_COUNT 2

typedef struct Onyx_Swapchain {
    VkDevice                     device;
    VkQueue                      presentQueue;
    Onyx_Frame                   frames[SWAPCHAIN_IMAGE_COUNT];
    uint32_t                     aovCount;
    Onyx_AovInfo                 aovInfos[ONYX_MAX_AOVS];
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
    Onyx_Memory*                 memory;
} Onyx_Swapchain;


Onyx_Swapchain* onyx_AllocSwapchain(void);
size_t          onyx_SizeOfSwapchain(void);
void            onyx_FreeSwapchain(Onyx_Swapchain* swapchain);
void onyx_CreateSwapchain(const Onyx_Instance*  instance,
                   Onyx_Memory*            memory,
                   Hell_EventQueue*        eventQueue,
                   const Hell_Window*      hellWindow,
                   VkImageUsageFlags       usageFlags,
                   uint32_t                extraAovCount,
                   Onyx_AovInfo            extraAovInfos[],
                   Onyx_Swapchain*         swapchain);
void            onyx_DestroySwapchain(const Onyx_Instance* instance, Onyx_Swapchain* swapchain);
unsigned        onyx_GetSwapchainWidth(const Onyx_Swapchain* swapchain);
unsigned        onyx_GetSwapchainHeight(const Onyx_Swapchain* swapchain);
VkExtent2D      onyx_GetSwapchainExtent(const Onyx_Swapchain* swapchain);
VkExtent3D      onyx_GetSwapchainExtent3D(const Onyx_Swapchain* swapchain);
VkFormat        onyx_GetSwapchainFormat(const Onyx_Swapchain* swapchain);

VkImageView onyx_GetSwapchainImageView(const Onyx_Swapchain* swapchain,
                                       int                   index);

unsigned onyx_GetSwapchainImageCount(const Onyx_Swapchain* swapchain);

// one thing to be careful of here is if this function fails the waitSemaphores
// will not be signalled.
bool onyx_PresentFrame(Onyx_Swapchain* swapchain, const uint32_t semaphoreCount,
                       VkSemaphore* waitSemaphores);

// fence can be a VK_NULL_HANDLE
const Onyx_Frame* onyx_AcquireSwapchainFrame(Onyx_Swapchain* swapchain, VkFence fence,
                                    VkSemaphore semaphore);

VkDeviceSize onyx_GetSwapchainImageSize(const Onyx_Swapchain* swapchain);

uint32_t onyx_GetSwapchainFrameCount(const Onyx_Swapchain*);
const Onyx_Frame* onyx_GetSwapchainFrames(const Onyx_Swapchain*);

uint32_t onyx_GetSwapchainPixelByteCount(const Onyx_Swapchain* swapchain);

#endif /* end of include guard: V_SWAPCHAIN_H */
