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

struct Obdn_V_Command;
typedef struct Obdn_Instance Obdn_Instance;

uint64_t obdn_SizeOfInstance(void);
Obdn_Instance* obdn_AllocInstance(void);

void obdn_CreateInstance(bool enableValidation, bool enableRayTracing, const int extcount,
                  const char* extensions[], Obdn_Instance* instance);
const VkInstance* obdn_GetVkInstance(const Obdn_Instance*);
void obdn_SubmitToQueue(const Obdn_Instance*, const VkCommandBuffer* buffer,
                        Obdn_V_QueueType, uint32_t queueIndex);
void obdn_SubmitToQueueWait(const Obdn_Instance*, const VkCommandBuffer* buffer,
                            Obdn_V_QueueType, uint32_t queueIndex);
void obdn_DestroyInstance(Obdn_Instance*);
void obdn_SubmitGraphicsCommands(const Obdn_Instance*,
                                 uint32_t      queueIndex,
                                 uint32_t      submitInfoCount,
                                 const VkSubmitInfo* submitInfos,
                                 VkFence             fence);
void obdn_SubmitTransferCommand(const Obdn_Instance*, uint32_t queueIndex,
                                VkPipelineStageFlags   waitDstStageMask,
                                const VkSemaphore*           pWaitSemephore,
                                VkFence                      fence,
                                const struct Obdn_V_Command* cmd);

void obdn_SubmitGraphicsCommand(const Obdn_Instance*, uint32_t queueIndex,
                                VkPipelineStageFlags waitDstStageMask,
                                uint32_t                   waitCount,
                                VkSemaphore waitSemephores[waitCount],
                                uint32_t          signalCount,
                                VkSemaphore signalSemphores[signalCount],
                                VkFence fence, VkCommandBuffer cmdBuf);

uint32_t obdn_GetQueueFamilyIndex(const Obdn_Instance*, Obdn_V_QueueType type);
VkDevice obdn_GetDevice(const Obdn_Instance*);
VkQueue  obdn_GetPresentQueue(const Obdn_Instance*);
VkPhysicalDevice obdn_GetPhysicalDevice(const Obdn_Instance*);

void obdn_PresentQueueWaitIdle(const Obdn_Instance*);

void obdn_DeviceWaitIdle(const Obdn_Instance*);

const VkPhysicalDeviceProperties*
obdn_GetPhysicalDeviceProperties(const Obdn_Instance*);

VkPhysicalDeviceRayTracingPipelinePropertiesKHR
obdn_GetPhysicalDeviceRayTracingProperties(const Obdn_Instance*);

VkPhysicalDeviceAccelerationStructurePropertiesKHR
obdn_GetPhysicalDeviceAccelerationStructureProperties(const Obdn_Instance*);


#endif /* end of include guard: V_VIDEO_H */
