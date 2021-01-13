#include "r_raytrace.h"
#include "v_video.h"
#include "r_render.h"
#include "v_memory.h"
#include "t_utils.h"
#include "v_command.h"
#include <assert.h>
#include "t_def.h"
#include <stdio.h>
#include <string.h>

typedef Tanto_V_BufferRegion          BufferRegion;
typedef Tanto_R_AccelerationStructure AccelerationStructure;
typedef Tanto_V_Command               Command;
typedef Tanto_R_ShaderBindingTable    ShaderBindingTable;

void tanto_r_BuildBlas(const Tanto_R_Primitive* prim, AccelerationStructure* blas)
{
    VkBufferDeviceAddressInfo addrInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = prim->vertexRegion.buffer,
    };

    const VkDeviceAddress vertAddr = vkGetBufferDeviceAddress(device, &addrInfo) + prim->vertexRegion.offset;

    addrInfo.buffer = prim->indexRegion.buffer;
    
    const VkDeviceAddress indexAddr = vkGetBufferDeviceAddress(device, &addrInfo) + prim->indexRegion.offset;

    const VkAccelerationStructureGeometryTrianglesDataKHR triData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat  = TANTO_VERT_POS_FORMAT,
        .vertexStride  = sizeof(Tanto_R_Attribute),
        .indexType     = TANTO_VERT_INDEX_TYPE,
        .maxVertex     = prim->vertexCount,
        .vertexData.deviceAddress = vertAddr,
        .indexData.deviceAddress = indexAddr,
        .transformData = 0
    };

    const VkAccelerationStructureGeometryKHR asGeom = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry.triangles = triData
    };

    // weird i need two
    VkAccelerationStructureBuildGeometryInfoKHR buildAS = {
        .sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount             = 1,
        .pGeometries               = &asGeom
    };

    VkAccelerationStructureBuildSizesInfoKHR buildSizes = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    const uint32_t numTrianlges = prim->indexCount / 3;

    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildAS, &numTrianlges, &buildSizes); 

    blas->bufferRegion = tanto_v_RequestBufferRegion(buildSizes.accelerationStructureSize, 
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
            TANTO_V_MEMORY_DEVICE_TYPE);

    const VkAccelerationStructureCreateInfoKHR accelStructInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = blas->bufferRegion.buffer,
        .offset = blas->bufferRegion.offset,
        .size   = blas->bufferRegion.size,
        .type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };

    V_ASSERT( vkCreateAccelerationStructureKHR(device, &accelStructInfo, NULL, &blas->handle) );

    Tanto_V_BufferRegion scratchBufferRegion = tanto_v_RequestBufferRegion(buildSizes.buildScratchSize, 
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
            TANTO_V_MEMORY_DEVICE_TYPE);

    buildAS.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildAS.dstAccelerationStructure = blas->handle;
    buildAS.scratchData.deviceAddress = tanto_v_GetBufferRegionAddress(&scratchBufferRegion);

    VkAccelerationStructureBuildRangeInfoKHR buildRange = {
        .firstVertex = 0,
        .primitiveCount = numTrianlges,
        .primitiveOffset = 0,
        .transformOffset = 0
    };

    const VkAccelerationStructureBuildRangeInfoKHR* ranges[1] = {&buildRange};

    Command cmd = tanto_v_CreateCommand(TANTO_V_QUEUE_GRAPHICS_TYPE);

    tanto_v_BeginCommandBuffer(cmd.buffer);

    vkCmdBuildAccelerationStructuresKHR(cmd.buffer, 1, &buildAS, ranges);

    tanto_v_EndCommandBuffer(cmd.buffer);

    tanto_v_SubmitAndWait(&cmd, 0);

    tanto_v_DestroyCommand(cmd);

    tanto_v_FreeBufferRegion(&scratchBufferRegion);
}

void tanto_r_BuildTlas(const AccelerationStructure* blas, AccelerationStructure* tlas)
{
    VkTransformMatrixKHR transform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0
    };

    //Mat4 transform = m_Ident_Mat4();

    //// instance transform member is a 4x3 row-major matrix
    //transform = m_Transpose_Mat4(&transform); // we don't need to because its identity, but leaving here to impress the point

    VkAccelerationStructureInstanceKHR instance = {
        .accelerationStructureReference = tanto_v_GetBufferRegionAddress(&blas->bufferRegion),
        .instanceCustomIndex = 0, // 0'th instance it
        .instanceShaderBindingTableRecordOffset = 0, 
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .mask = 0xFF,
        .transform = transform
    };

    BufferRegion instBuffer = tanto_v_RequestBufferRegion(1 * sizeof(VkAccelerationStructureInstanceKHR),
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            TANTO_V_MEMORY_HOST_TYPE);

    assert(instBuffer.hostData);
    memcpy(instBuffer.hostData, &instance, 1 * sizeof(instance));

    const VkAccelerationStructureGeometryInstancesDataKHR instanceData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .data.deviceAddress = tanto_v_GetBufferRegionAddress(&instBuffer),
        .arrayOfPointers = VK_FALSE
    };

    const VkAccelerationStructureGeometryDataKHR geometry = {
        .instances = instanceData
    };

    const VkAccelerationStructureGeometryKHR topAsGeometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometry = geometry,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR // may not need
    };

    VkAccelerationStructureBuildGeometryInfoKHR topAsInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .geometryCount = 1,
        .pGeometries = &topAsGeometry,
    };

    const uint32_t maxPrimCount = 1;

    VkAccelerationStructureBuildSizesInfoKHR buildSizes = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &topAsInfo, &maxPrimCount, &buildSizes); 

    tlas->bufferRegion = tanto_v_RequestBufferRegion(buildSizes.accelerationStructureSize, 
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            TANTO_V_MEMORY_DEVICE_TYPE);

    const VkAccelerationStructureCreateInfoKHR asCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .buffer = tlas->bufferRegion.buffer,
        .offset = tlas->bufferRegion.offset,
        .size   = tlas->bufferRegion.size,
    };

    V_ASSERT( vkCreateAccelerationStructureKHR(device, &asCreateInfo, NULL, &tlas->handle) );

    // allocate and bind memory

    BufferRegion scratchBuffer = tanto_v_RequestBufferRegion(buildSizes.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            TANTO_V_MEMORY_DEVICE_TYPE);

    topAsInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    topAsInfo.dstAccelerationStructure = tlas->handle;
    topAsInfo.scratchData.deviceAddress = tanto_v_GetBufferRegionAddress(&scratchBuffer);

    const VkAccelerationStructureBuildRangeInfoKHR buildRange = {
        .firstVertex = 0,
        .primitiveCount = 1,
        .primitiveOffset = 0,
        .transformOffset = 0
    };

    const VkAccelerationStructureBuildRangeInfoKHR* ranges[1] = {&buildRange};

    Command cmd = tanto_v_CreateCommand(TANTO_V_QUEUE_GRAPHICS_TYPE);

    tanto_v_BeginCommandBuffer(cmd.buffer);

    vkCmdBuildAccelerationStructuresKHR(cmd.buffer, 1, &topAsInfo, ranges);

    tanto_v_EndCommandBuffer(cmd.buffer);

    tanto_v_SubmitAndWait(&cmd, TANTO_V_QUEUE_GRAPHICS_TYPE);

    tanto_v_DestroyCommand(cmd);
    tanto_v_FreeBufferRegion(&scratchBuffer);
    tanto_v_FreeBufferRegion(&instBuffer);
}

void tanto_r_CreateShaderBindingTable(const uint32_t groupCount, const VkPipeline pipeline, ShaderBindingTable* sbt)
{
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtprops = tanto_v_GetPhysicalDeviceRayTracingProperties();
    const uint32_t groupHandleSize = rtprops.shaderGroupHandleSize;
    const uint32_t baseAlignment = rtprops.shaderGroupBaseAlignment;
    const uint32_t sbtSize = groupCount * baseAlignment; // 3 shader groups: raygen, miss, closest hit

    uint8_t shaderHandleData[sbtSize];

    printf("ShaderGroup handle size: %d\n", groupHandleSize);
    printf("ShaderGroup base alignment: %d\n", baseAlignment);
    printf("ShaderGroups total size   : %d\n", sbtSize);

    VkResult r;
    r = vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleData);
    assert( VK_SUCCESS == r );
    sbt->bufferRegion = tanto_v_RequestBufferRegionAligned(sbtSize, baseAlignment, TANTO_V_MEMORY_HOST_TYPE);
    sbt->groupCount = groupCount;

    uint8_t* pSrc    = shaderHandleData;
    uint8_t* pTarget = sbt->bufferRegion.hostData;

    for (int i = 0; i < groupCount; i++) 
    {
        memcpy(pTarget, pSrc + i * groupHandleSize, groupHandleSize);
        pTarget += baseAlignment;
    }

    printf("Created shader binding table\n");
}

void tanto_r_DestroyAccelerationStruct(AccelerationStructure* as)
{
    vkDestroyAccelerationStructureKHR(device, as->handle, NULL);
    tanto_v_FreeBufferRegion(&as->bufferRegion);
}

