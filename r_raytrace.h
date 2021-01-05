#ifndef R_RAYTRACE_H
#define R_RAYTRACE_H

#include "v_video.h"
#include "r_render.h"
#include "r_geo.h"

typedef struct {
    VkAccelerationStructureKHR handle;
    VkBuffer        buffer;
    VkDeviceSize    size;
    VkDeviceMemory  memory;
    VkDeviceAddress address;
} AccelerationStructure;

extern AccelerationStructure bottomLevelAS;
extern AccelerationStructure topLevelAS;

void tanto_r_InitRayTracing(void);
void tanto_r_BuildBlas(const Tanto_R_Primitive* prim);
void tanto_r_BuildTlas(void);
void tanto_r_RayTraceDestroyAccelStructs(void);
void tanto_r_RayTraceCleanUp(void);

#endif /* end of include guard: R_RAYTRACE_H */
