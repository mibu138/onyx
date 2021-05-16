#ifndef V_SWAPCHAIN_H
#define V_SWAPCHAIN_H

#include "v_def.h"
#include "v_memory.h"
#include "v_command.h"
#include "types.h"
#include <hell/window.h>

typedef Obdn_V_Image   Obdn_R_Frame;

typedef void (*Obdn_R_SwapchainRecreationFn)(void);
typedef struct Obdn_Swapchain Obdn_Swapchain;

Obdn_Swapchain* obdn_AllocSwapchain(void);
size_t          obdn_SizeOfSwapchain(void);
void            obdn_FreeSwapchain(Obdn_Swapchain* swapchain);
void            obdn_InitSwapchain(Obdn_Swapchain*         swapchain,
                                   const VkImageUsageFlags swapImageUsageFlags_,
                                   const Hell_Window*      hellWindow);
void            obdn_ShutdownSwapchain(Obdn_Swapchain* swapchain);
unsigned        obdn_GetSwapchainWidth(const Obdn_Swapchain* swapchain);
unsigned        obdn_GetSwapchainHeight(const Obdn_Swapchain* swapchain);
VkExtent2D      obdn_GetSwapchainExtent(const Obdn_Swapchain* swapchain);
VkExtent3D      obdn_GetSwapchainExtent3D(const Obdn_Swapchain* swapchain);
VkFormat        obdn_GetSwapchainFormat(const Obdn_Swapchain* swapchain);

VkImageView obdn_GetSwapchainImageView(const Obdn_Swapchain* swapchain,
                                       int                   index);

unsigned obdn_GetSwapchainImageCount(const Obdn_Swapchain* swapchain);
const VkImageView* obdn_GetSwapchainImageViews(const Obdn_Swapchain* swapchain);

// one thing to be careful of here is if this function fails the waitSemaphores
// will not be signalled.
bool obdn_PresentFrame(const Obdn_Swapchain* swapchain, const uint32_t semaphoreCount,
                       VkSemaphore* waitSemaphores);

unsigned obdn_AcquireSwapchainImage(Obdn_Swapchain* swapchain, VkFence* fence,
                                    VkSemaphore* semaphore, bool* dirty);

VkImage obdn_GetSwapchainImage(const Obdn_Swapchain* swapchain, uint32_t index);

VkDeviceSize obdn_GetSwapchainImageSize(const Obdn_Swapchain* swapchain);

uint32_t obdn_GetSwapchainPixelByteCount(const Obdn_Swapchain* swapchain);

#endif /* end of include guard: V_SWAPCHAIN_H */
