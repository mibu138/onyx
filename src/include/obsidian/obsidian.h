#ifndef OBSIDIAN_H
#define OBSIDIAN_H

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

typedef VkDevice Obdn_Device;

typedef struct {
    Obdn_Instance* instance;
    Obdn_Memory*   memory;
    Obdn_Device    device;   
} Obdn_Orb;

static inline int obdn_CreateOrb(const Obdn_InstanceParms* ip,
    const uint32_t hostGraphicsBufferMB,
    const uint32_t deviceGraphicsBufferMB,
    const uint32_t deviceGraphicsImageMB, const uint32_t hostTransferBufferMB,
    const uint32_t deviceExternalGraphicsImageMB, 
    Obdn_Orb* orb)
{
    orb->instance = obdn_AllocInstance();
    orb->memory   = obdn_AllocMemory();
    obdn_CreateInstance(ip, orb->instance);
    obdn_CreateMemory(orb->instance, hostTransferBufferMB,
        deviceGraphicsBufferMB, deviceGraphicsImageMB,
        hostTransferBufferMB, deviceExternalGraphicsImageMB, orb->memory);
    orb->device = obdn_GetDevice(orb->instance);
    return 0;
}

#endif /* end of include guard: OBSIDIAN_H */
