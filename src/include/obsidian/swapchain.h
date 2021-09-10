#ifndef V_SWAPCHAIN_H
#define V_SWAPCHAIN_H

#include "def.h"
#include "memory.h"
#include "command.h"
#include "framebuffer.h"
#include "types.h"
#include <hell/window.h>

typedef Obdn_Image   Obdn_R_Frame;

typedef void (*Obdn_R_SwapchainRecreationFn)(void);
typedef struct Obdn_Swapchain Obdn_Swapchain;

Obdn_Swapchain* obdn_AllocSwapchain(void);
size_t          obdn_SizeOfSwapchain(void);
void            obdn_FreeSwapchain(Obdn_Swapchain* swapchain);
void obdn_CreateSwapchain(const Obdn_Instance*  instance,
                   Obdn_Memory*            memory,
                   Hell_EventQueue*        eventQueue,
                   const Hell_Window*      hellWindow,
                   VkImageUsageFlags       usageFlags,
                   uint32_t                extraAovCount,
                   Obdn_AovInfo            extraAovInfos[],
                   Obdn_Swapchain*         swapchain);
void            obdn_DestroySwapchain(const Obdn_Instance* instance, Obdn_Swapchain* swapchain);
unsigned        obdn_GetSwapchainWidth(const Obdn_Swapchain* swapchain);
unsigned        obdn_GetSwapchainHeight(const Obdn_Swapchain* swapchain);
VkExtent2D      obdn_GetSwapchainExtent(const Obdn_Swapchain* swapchain);
VkExtent3D      obdn_GetSwapchainExtent3D(const Obdn_Swapchain* swapchain);
VkFormat        obdn_GetSwapchainFormat(const Obdn_Swapchain* swapchain);

VkImageView obdn_GetSwapchainImageView(const Obdn_Swapchain* swapchain,
                                       int                   index);

unsigned obdn_GetSwapchainImageCount(const Obdn_Swapchain* swapchain);

// one thing to be careful of here is if this function fails the waitSemaphores
// will not be signalled.
bool obdn_PresentFrame(Obdn_Swapchain* swapchain, const uint32_t semaphoreCount,
                       VkSemaphore* waitSemaphores);

const Obdn_Framebuffer* obdn_AcquireSwapchainFramebuffer(Obdn_Swapchain* swapchain, VkFence* fence,
                                    VkSemaphore* semaphore);

VkDeviceSize obdn_GetSwapchainImageSize(const Obdn_Swapchain* swapchain);

uint32_t obdn_GetSwapchainFramebufferCount(const Obdn_Swapchain*);
const Obdn_Framebuffer* obdn_GetSwapchainFramebuffers(const Obdn_Swapchain*);

uint32_t obdn_GetSwapchainPixelByteCount(const Obdn_Swapchain* swapchain);

#endif /* end of include guard: V_SWAPCHAIN_H */
