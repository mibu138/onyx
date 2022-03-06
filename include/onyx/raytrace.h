#ifndef R_RAYTRACE_H
#define R_RAYTRACE_H

#include "video.h"
#include "render.h"
#include "geo.h"

typedef struct {
    VkAccelerationStructureKHR handle;
    Onyx_BufferRegion        bufferRegion;
} Onyx_AccelerationStructure;

typedef struct {
    Onyx_BufferRegion             bufferRegion;
    uint32_t                        groupCount;
    VkStridedDeviceAddressRegionKHR raygenTable;
    VkStridedDeviceAddressRegionKHR missTable;
    VkStridedDeviceAddressRegionKHR hitTable;
    VkStridedDeviceAddressRegionKHR callableTable;
} Onyx_ShaderBindingTable;

void onyx_BuildBlas(Onyx_Memory*, const Onyx_Geometry* prim, Onyx_AccelerationStructure* blas);
void onyx_BuildTlas(Onyx_Memory*, const uint32_t count, const Onyx_AccelerationStructure blasses[],
        const Coal_Mat4 xforms[],
        Onyx_AccelerationStructure* tlas);
void onyx_CreateShaderBindingTable(Onyx_Memory*, const uint32_t groupCount, const VkPipeline pipeline, Onyx_ShaderBindingTable* sbt);
void onyx_DestroyAccelerationStruct(VkDevice device, Onyx_AccelerationStructure* as);
void onyx_DestroyShaderBindingTable(Onyx_ShaderBindingTable* sb);

#endif /* end of include guard: R_RAYTRACE_H */
