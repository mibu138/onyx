/*
v_video.c
 */
#ifndef TANTO_V_VIDEO_H
#define TANTO_V_VIDEO_H

#include "v_def.h"

typedef enum {
    TANTO_V_QUEUE_GRAPHICS_TYPE,
    TANTO_V_QUEUE_TRANSFER_TYPE,
    TANTO_V_QUEUE_COMPUTE_TYPE,
} Tanto_V_QueueType;

extern VkDevice                   device;
extern VkPhysicalDevice           physicalDevice;
extern VkPhysicalDeviceProperties deviceProperties;

struct Tanto_V_Command;

const VkInstance* tanto_v_Init(void);
void              tanto_v_InitSurfaceXcb(xcb_connection_t* connection, xcb_window_t window);
void              tanto_v_SubmitToQueue(const VkCommandBuffer* buffer, const Tanto_V_QueueType, const uint32_t queueIndex);
void              tanto_v_SubmitToQueueWait(const VkCommandBuffer* buffer, const Tanto_V_QueueType, const uint32_t queueIndex);
void              tanto_v_AcquireSwapImage(uint32_t* pImageIndex);
uint32_t          tanto_v_GetQueueFamilyIndex(Tanto_V_QueueType type);
VkDevice          tanto_v_GetDevice(void);
VkQueue           tanto_v_GetPresentQueue(void);
void              tanto_v_CleanUp(void);
VkSurfaceKHR      tanto_v_GetSurface(void);
void              tanto_v_SubmitGraphicsCommands(const uint32_t queueIndex, const uint32_t submitInfoCount, 
                     const VkSubmitInfo* submitInfos, VkFence fence);
void              tanto_v_SubmitGraphicsCommand(const uint32_t queueIndex, const VkPipelineStageFlags waitDstStageMask, 
                                    const VkSemaphore* pWaitSemephore, VkFence fence, const struct Tanto_V_Command* cmd);
void              tanto_v_SubmitTransferCommand(const uint32_t queueIndex, 
                     const VkPipelineStageFlags waitDstStageMask, const VkSemaphore* pWaitSemephore, 
                     VkFence fence, const struct Tanto_V_Command* cmd);

VkPhysicalDeviceRayTracingPipelinePropertiesKHR tanto_v_GetPhysicalDeviceRayTracingProperties(void);

#endif /* end of include guard: V_VIDEO_H */
