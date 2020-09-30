#ifndef VKVIEWER_H
#define VKVIEWER_H

#include "v_vulkan.h"

const VkInstance* t_InitVulkan(void);
void t_InitVulkanSwapchain(VkSurfaceKHR* surface);
void t_StartViewer(void);
void t_CleanUp(void);

#endif /* end of include guard: VKVIEWER_H */
