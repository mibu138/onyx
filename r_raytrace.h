#ifndef R_RAYTRACE_H
#define R_RAYTRACE_H

#include "v_video.h"
#include "r_render.h"
#include "r_geo.h"

typedef struct blas {
    VkAccelerationStructureGeometryKHR               asGeometry;
    VkAccelerationStructureCreateGeometryTypeInfoKHR asCreateGeo;
    VkAccelerationStructureBuildOffsetInfoKHR        asBuildOffset;
} Blas;

extern VkAccelerationStructureKHR bottomLevelAS;
extern VkAccelerationStructureKHR topLevelAS;

void r_InitRayTracing(void);
void r_BuildBlas(const Mesh* mesh);
void r_BuildTlas(void);
void r_RayTraceCleanUp(void);

#endif /* end of include guard: R_RAYTRACE_H */
