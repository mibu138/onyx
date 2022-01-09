/*
video.c
 */
#ifndef OBDN_V_VIDEO_H
#define OBDN_V_VIDEO_H

#include "def.h"
#include "types.h"

typedef enum {
    OBDN_V_QUEUE_GRAPHICS_TYPE,
    OBDN_V_QUEUE_TRANSFER_TYPE,
    OBDN_V_QUEUE_COMPUTE_TYPE,
} Obdn_V_QueueType;

typedef enum Obdn_SurfaceType {
    OBDN_SURFACE_TYPE_NO_WINDOW,
    OBDN_SURFACE_TYPE_XCB,
    OBDN_SURFACE_TYPE_WIN32
} Obdn_SurfaceType;

struct Obdn_V_Command;
typedef struct Obdn_Instance Obdn_Instance;

uint64_t obdn_SizeOfInstance(void);
Obdn_Instance* obdn_AllocInstance(void);

// enabling/disabling validation and/or raytracing 
// will automatically enable the required layers and 
// extensions for those to be used effectively
typedef struct Obdn_InstanceParms {
    bool                                disableValidation;
    bool                                enableRayTracing;
    Obdn_SurfaceType                    surfaceType;
    uint32_t                            enabledInstanceLayerCount;
    const char**                        ppEnabledInstanceLayerNames;
    uint32_t                            enabledInstanceExentensionCount;
    const char**                        ppEnabledInstanceExtensionNames;
    uint32_t                            validationFeaturesCount;
    const VkValidationFeatureEnableEXT* pValidationFeatures;
    uint32_t                            enabledDeviceExtensionCount;
    const char**                        ppEnabledDeviceExtensionNames;
} Obdn_InstanceParms;

Obdn_Result obdn_CreateInstance(const Obdn_InstanceParms* parms, Obdn_Instance* instance);
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
                                VkSemaphore waitSemephores[],
                                uint32_t          signalCount,
                                VkSemaphore signalSemphores[],
                                VkFence fence, VkCommandBuffer cmdBuf);

uint32_t obdn_GetQueueFamilyIndex(const Obdn_Instance*, Obdn_V_QueueType type);
VkDevice obdn_GetDevice(const Obdn_Instance*);
VkQueue  obdn_GetPresentQueue(const Obdn_Instance*);
VkQueue  obdn_GetGrahicsQueue(const Obdn_Instance*, u32 index);
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
