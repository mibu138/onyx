/*
v_video.c
 */
#ifndef TANTO_V_VIDEO_H
#define TANTO_V_VIDEO_H

#include "v_def.h"

typedef enum {
    TANTO_V_QUEUE_GRAPHICS_TYPE,
    TANTO_V_QUEUE_COMPUTE_TYPE,
} Tanto_V_QueueType;

extern VkDevice         device;
extern VkPhysicalDevice physicalDevice;
extern uint32_t graphicsQueueFamilyIndex;
extern VkQueue  graphicsQueues[TANTO_G_QUEUE_COUNT];
extern VkQueue  presentQueue;
extern VkSurfaceKHR* pSurface;

extern VkPhysicalDeviceProperties deviceProperties;

const VkInstance* tanto_v_Init(void);
void tanto_v_InitSurfaceXcb(xcb_connection_t* connection, xcb_window_t window);
void tanto_v_SubmitToQueue(const VkCommandBuffer* buffer, const Tanto_V_QueueType, const uint32_t queueIndex);
void tanto_v_SubmitToQueueWait(const VkCommandBuffer* buffer, const Tanto_V_QueueType, const uint32_t queueIndex);
void tanto_v_AcquireSwapImage(uint32_t* pImageIndex);
uint32_t tanto_v_GetQueueFamilyIndex(Tanto_V_QueueType type);
VkDevice tanto_v_GetDevice(void);
void tanto_v_CleanUp(void);

VkPhysicalDeviceRayTracingPropertiesKHR tanto_v_GetPhysicalDeviceRayTracingProperties(void);

#endif /* end of include guard: V_VIDEO_H */
