/*
v_video.c
 */
#ifndef OBDN_V_VIDEO_H
#define OBDN_V_VIDEO_H

#include "v_def.h"

typedef enum {
    OBDN_V_QUEUE_GRAPHICS_TYPE,
    OBDN_V_QUEUE_TRANSFER_TYPE,
    OBDN_V_QUEUE_COMPUTE_TYPE,
} Obdn_V_QueueType;

extern VkDevice                   device;
extern VkPhysicalDevice           physicalDevice;
extern VkPhysicalDeviceProperties deviceProperties;

struct Obdn_V_Command;

const VkInstance* obdn_v_Init(void);
void              obdn_v_InitSurfaceXcb(xcb_connection_t* connection, xcb_window_t window);
void              obdn_v_SubmitToQueue(const VkCommandBuffer* buffer, const Obdn_V_QueueType, const uint32_t queueIndex);
void              obdn_v_SubmitToQueueWait(const VkCommandBuffer* buffer, const Obdn_V_QueueType, const uint32_t queueIndex);
void              obdn_v_AcquireSwapImage(uint32_t* pImageIndex);
uint32_t          obdn_v_GetQueueFamilyIndex(Obdn_V_QueueType type);
VkDevice          obdn_v_GetDevice(void);
VkQueue           obdn_v_GetPresentQueue(void);
void              obdn_v_CleanUp(void);
VkSurfaceKHR      obdn_v_GetSurface(void);
void              obdn_v_SubmitGraphicsCommands(const uint32_t queueIndex, const uint32_t submitInfoCount, 
                     const VkSubmitInfo* submitInfos, VkFence fence);
void obdn_v_SubmitGraphicsCommand(const uint32_t queueIndex, 
        const VkPipelineStageFlags waitDstStageMask, const VkSemaphore waitSemephore, 
        const VkSemaphore signalSemphore, VkFence fence, const VkCommandBuffer cmdBuf);
void              obdn_v_SubmitTransferCommand(const uint32_t queueIndex, 
                     const VkPipelineStageFlags waitDstStageMask, const VkSemaphore* pWaitSemephore, 
                     VkFence fence, const struct Obdn_V_Command* cmd);

VkPhysicalDeviceRayTracingPipelinePropertiesKHR obdn_v_GetPhysicalDeviceRayTracingProperties(void);

#endif /* end of include guard: V_VIDEO_H */
