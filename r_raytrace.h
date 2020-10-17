#ifndef R_RAYTRACE_H
#define R_RAYTRACE_H

#include "v_video.h"
#include "r_render.h"
#include "r_geo.h"

extern VkAccelerationStructureKHR bottomLevelAS;
extern VkAccelerationStructureKHR topLevelAS;

void tanto_r_InitRayTracing(void);
void tanto_r_BuildBlas(const Tanto_R_Mesh* mesh);
void tanto_r_BuildTlas(void);
void tanto_r_RayTraceDestroyAccelStructs(void);
void tanto_r_RayTraceCleanUp(void);

#endif /* end of include guard: R_RAYTRACE_H */
