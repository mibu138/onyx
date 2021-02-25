#ifndef OBDN_R_INIT_H
#define OBDN_R_INIT_H

#include "v_def.h"
#include "v_memory.h"
#include <coal/coal.h>
#include "v_command.h"
#include "s_scene.h"

#define OBDN_VERT_POS_FORMAT VK_FORMAT_R32G32B32_SFLOAT
#define OBDN_VERT_INDEX_TYPE VK_INDEX_TYPE_UINT32

typedef Obdn_V_Image Obdn_R_Frame;

typedef void (*Obdn_R_SwapchainRecreationFn)(void);

VkFormat obdn_r_GetOffscreenColorFormat(void);
VkFormat obdn_r_GetDepthFormat(void);
VkFormat obdn_r_GetSwapFormat(void);

void                obdn_r_Init(const VkImageUsageFlags swapImageUsageFlags, bool offscreenSwapchain);
void                obdn_r_DrawScene(const VkCommandBuffer cmdBuf, const Obdn_S_Scene* scene);
bool                obdn_r_PresentFrame(VkSemaphore waitSemaphore);
void                obdn_r_CleanUp(void);
void                obdn_r_RecreateSwapchain(void); // in case we are doing offscreen rendering. call on resizing.
void                obdn_r_RegisterSwapchainRecreationFn(Obdn_R_SwapchainRecreationFn fn);
void                obdn_r_UnregisterSwapchainRecreateFn(Obdn_R_SwapchainRecreationFn fn);
void                obdn_r_RecreateSwapchain(void);
const Obdn_R_Frame* obdn_r_GetFrame(const int8_t index);
const uint32_t      obdn_r_GetCurrentFrameIndex(void);
const uint32_t      obdn_r_RequestFrame(void); // returns available frame index.

#endif /* end of include guard: R_INIT_H */
