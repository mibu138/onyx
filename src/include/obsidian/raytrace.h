#ifndef R_RAYTRACE_H
#define R_RAYTRACE_H

#include "video.h"
#include "render.h"
#include "geo.h"

typedef struct {
    VkAccelerationStructureKHR handle;
    Obdn_BufferRegion        bufferRegion;
} Obdn_R_AccelerationStructure;

typedef struct {
    Obdn_BufferRegion             bufferRegion;
    uint32_t                        groupCount;
    VkStridedDeviceAddressRegionKHR raygenTable;
    VkStridedDeviceAddressRegionKHR missTable;
    VkStridedDeviceAddressRegionKHR hitTable;
    VkStridedDeviceAddressRegionKHR callableTable;
} Obdn_R_ShaderBindingTable;

void obdn_BuildBlas(Obdn_Memory*, const Obdn_Geometry* prim, Obdn_R_AccelerationStructure* blas);
void obdn_BuildTlas(Obdn_Memory*, const uint32_t count, const Obdn_R_AccelerationStructure blasses[],
        const Coal_Mat4 xforms[],
        Obdn_R_AccelerationStructure* tlas);
void obdn_CreateShaderBindingTable(Obdn_Memory*, const uint32_t groupCount, const VkPipeline pipeline, Obdn_R_ShaderBindingTable* sbt);
void obdn_DestroyAccelerationStruct(VkDevice device, Obdn_R_AccelerationStructure* as);
void obdn_DestroyShaderBindingTable(Obdn_R_ShaderBindingTable* sb);

#endif /* end of include guard: R_RAYTRACE_H */
