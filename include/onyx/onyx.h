#ifndef ONYX_H
#define ONYX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "video.h"
#include "memory.h"
#include "image.h"
#include "swapchain.h"
#include "scene.h"
#include "render.h"
#include "raytrace.h"
#include "renderpass.h"
#include "geo.h"
#include "file.h"
#include "pipeline.h"

typedef VkDevice Onyx_Device;

typedef struct {
    Onyx_Instance* instance;
    Onyx_Memory*   memory;
    Onyx_Device    device;
} Onyx_Orb;

typedef Onyx_Orb OnyxContext;

static inline int onyx_CreateOrb(const Onyx_InstanceParms* ip,
    const uint32_t hostGraphicsBufferMB,
    const uint32_t deviceGraphicsBufferMB,
    const uint32_t deviceGraphicsImageMB, const uint32_t hostTransferBufferMB,
    const uint32_t deviceExternalGraphicsImageMB,
    Onyx_Orb* orb)
{
    orb->instance = onyx_AllocInstance();
    orb->memory   = onyx_AllocMemory();
    onyx_CreateInstance(ip, orb->instance);
    onyx_CreateMemory(orb->instance, hostGraphicsBufferMB,
        deviceGraphicsBufferMB, deviceGraphicsImageMB,
        hostTransferBufferMB, deviceExternalGraphicsImageMB, orb->memory);
    orb->device = onyx_GetDevice(orb->instance);
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: ONYX_H */
