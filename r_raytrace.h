#ifndef R_RAYTRACE_H
#define R_RAYTRACE_H

#include "v_video.h"
#include "r_render.h"
#include "r_geo.h"

typedef struct {
    VkAccelerationStructureKHR handle;
    Tanto_V_BufferRegion       bufferRegion;
} Tanto_R_AccelerationStructure;

typedef struct {
    Tanto_V_BufferRegion bufferRegion;
    uint32_t             groupCount;
} Tanto_R_ShaderBindingTable;

void tanto_r_BuildBlas(const Tanto_R_Primitive* prim, Tanto_R_AccelerationStructure* blas);
void tanto_r_BuildTlas(const Tanto_R_AccelerationStructure* blas, Tanto_R_AccelerationStructure* tlas);
void tanto_r_CreateShaderBindingTable(const uint32_t groupCount, const VkPipeline pipeline, Tanto_R_ShaderBindingTable* sbt);
void tanto_r_DestroyAccelerationStruct(Tanto_R_AccelerationStructure* as);

#endif /* end of include guard: R_RAYTRACE_H */
