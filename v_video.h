/*
v_video.c
 */
#ifndef V_VIDEO_H
#define V_VIDEO_H

#include "v_def.h"

typedef enum {
    V_QUEUE_TYPE_GRAPHICS,
    V_QUEUE_TYPE_COMPUTE
} V_QueueType;

extern VkDevice         device;
extern VkPhysicalDevice physicalDevice;
extern uint32_t graphicsQueueFamilyIndex;
extern VkQueue  graphicsQueues[G_QUEUE_COUNT];
extern VkQueue  presentQueue;

extern VkImage        swapchainImages[FRAME_COUNT];
extern const VkFormat swapFormat;
extern VkSwapchainKHR   swapchain;
extern VkSemaphore    imageAcquiredSemaphores[FRAME_COUNT];
extern uint64_t       frameCounter;

const VkInstance* v_Init(void);
void v_InitSwapchain(VkSurfaceKHR* surface);
void v_SubmitToQueue(const VkCommandBuffer* buffer, const V_QueueType, const uint32_t queueIndex);
void v_SubmitToQueueWait(const VkCommandBuffer* buffer, const V_QueueType, const uint32_t queueIndex);
void v_AcquireSwapImage(uint32_t* pImageIndex);
void v_CleanUp(void);

VkPhysicalDeviceRayTracingPropertiesKHR v_GetPhysicalDeviceRayTracingProperties(void);

#endif /* end of include guard: V_VIDEO_H */
