/*
video.c
 */
#ifndef ONYX_V_VIDEO_H
#define ONYX_V_VIDEO_H

#include "def.h"
#include "types.h"

typedef enum {
    ONYX_V_QUEUE_GRAPHICS_TYPE,
    ONYX_V_QUEUE_TRANSFER_TYPE,
    ONYX_V_QUEUE_COMPUTE_TYPE,
} Onyx_V_QueueType;

typedef enum Onyx_SurfaceType {
    ONYX_SURFACE_TYPE_NO_WINDOW,
    ONYX_SURFACE_TYPE_XCB,
    ONYX_SURFACE_TYPE_WIN32
} Onyx_SurfaceType;

struct Onyx_V_Command;
typedef struct Onyx_Instance Onyx_Instance;

uint64_t onyx_SizeOfInstance(void);
Onyx_Instance* onyx_AllocInstance(void);

// enabling/disabling validation and/or raytracing
// will automatically enable the required layers and
// extensions for those to be used effectively
typedef struct Onyx_InstanceParms {
    bool                                disableValidation;
    bool                                enableRayTracing;
    Onyx_SurfaceType                    surfaceType;
    uint32_t                            enabledInstanceLayerCount;
    const char**                        ppEnabledInstanceLayerNames;
    uint32_t                            enabledInstanceExentensionCount;
    const char**                        ppEnabledInstanceExtensionNames;
    uint32_t                            validationFeaturesCount;
    const VkValidationFeatureEnableEXT* pValidationFeatures;
    uint32_t                            enabledDeviceExtensionCount;
    const char**                        ppEnabledDeviceExtensionNames;
} Onyx_InstanceParms;

void onyx_CreateInstance(const Onyx_InstanceParms* parms, Onyx_Instance* instance);
const VkInstance* onyx_GetVkInstance(const Onyx_Instance*);
void onyx_SubmitToQueue(const Onyx_Instance*, const VkCommandBuffer* buffer,
                        Onyx_V_QueueType, uint32_t queueIndex);
void onyx_SubmitToQueueWait(const Onyx_Instance*, const VkCommandBuffer* buffer,
                            Onyx_V_QueueType, uint32_t queueIndex);

void
onyx_QueueSubmit(
    VkQueue                                     queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo*                         pSubmits,
    VkFence                                     fence);
void onyx_DestroyInstance(Onyx_Instance*);
void onyx_SubmitGraphicsCommands(const Onyx_Instance*,
                                 uint32_t      queueIndex,
                                 uint32_t      submitInfoCount,
                                 const VkSubmitInfo* submitInfos,
                                 VkFence             fence);
void onyx_SubmitTransferCommand(const Onyx_Instance*, uint32_t queueIndex,
                                VkPipelineStageFlags   waitDstStageMask,
                                const VkSemaphore*           pWaitSemephore,
                                VkFence                      fence,
                                const struct Onyx_V_Command* cmd);

void onyx_SubmitGraphicsCommand(const Onyx_Instance*, uint32_t queueIndex,
                                VkPipelineStageFlags waitDstStageMask,
                                uint32_t                   waitCount,
                                VkSemaphore waitSemephores[],
                                uint32_t          signalCount,
                                VkSemaphore signalSemphores[],
                                VkFence fence, VkCommandBuffer cmdBuf);

uint32_t onyx_GetQueueFamilyIndex(const Onyx_Instance*, Onyx_V_QueueType type);
VkDevice onyx_GetDevice(const Onyx_Instance*);
VkQueue  onyx_GetPresentQueue(const Onyx_Instance*);
VkQueue  onyx_GetGraphicsQueue(const Onyx_Instance*, u32 index);
VkQueue  onyx_GetTransferQueue(const Onyx_Instance*, u32 index);
VkPhysicalDevice onyx_GetPhysicalDevice(const Onyx_Instance*);

void onyx_PresentQueueWaitIdle(const Onyx_Instance*);

void onyx_DeviceWaitIdle(const Onyx_Instance*);

const VkPhysicalDeviceProperties*
onyx_GetPhysicalDeviceProperties(const Onyx_Instance*);

void
onyx_QueueSubmit2(
    VkQueue                                     queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo2*                        pSubmits,
    VkFence                                     fence);

VkPhysicalDeviceRayTracingPipelinePropertiesKHR
onyx_GetPhysicalDeviceRayTracingProperties(const Onyx_Instance*);

VkPhysicalDeviceAccelerationStructurePropertiesKHR
onyx_GetPhysicalDeviceAccelerationStructureProperties(const Onyx_Instance*);


#endif /* end of include guard: V_VIDEO_H */
