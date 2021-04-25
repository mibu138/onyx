#ifndef V_SWAPCHAIN_H
#define V_SWAPCHAIN_H

#include "v_def.h"
#include "v_memory.h"
#include "types.h"
#include <hell/window.h>

typedef Obdn_V_Image Obdn_R_Frame;

typedef void (*Obdn_R_SwapchainRecreationFn)(void);

void                obdn_v_InitSwapchain(const VkImageUsageFlags swapImageUsageFlags_, const Hell_Window* hellWindow);
bool                obdn_v_PresentFrame(VkSemaphore waitSemaphore);
void                obdn_v_CleanUpSwapchain(void);
void                obdn_v_RegisterSwapchainRecreationFn(Obdn_R_SwapchainRecreationFn fn);
void                obdn_v_UnregisterSwapchainRecreateFn(Obdn_R_SwapchainRecreationFn fn);
const uint32_t      obdn_v_RequestFrame(Obdn_Mask* dirtyFlag, uint16_t window[2]);
VkFormat            obdn_v_GetSwapFormat(void);
const Obdn_R_Frame* obdn_v_GetFrame(const int8_t index);
const uint32_t      obdn_v_GetCurrentFrameIndex(void);
VkExtent2D          obdn_v_GetSwapExtent(void);

#endif /* end of include guard: V_SWAPCHAIN_H */
