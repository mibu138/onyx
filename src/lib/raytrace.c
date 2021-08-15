#define COAL_SIMPLE_TYPE_NAMES
#include "raytrace.h"
#include "video.h"
#include "render.h"
#include "memory.h"
#include "command.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <hell/debug.h>
#include "dtags.h"
#include "private.h"
#include "coal/types.h"
#include "coal/linalg.h"

typedef Obdn_V_BufferRegion          BufferRegion;
typedef Obdn_R_AccelerationStructure AccelerationStructure;
typedef Obdn_V_Command               Command;
typedef Obdn_R_ShaderBindingTable    ShaderBindingTable;

#define DPRINT(fmt, ...) hell_DebugPrint(OBDN_DEBUG_TAG_RAYTRACE, fmt, ##__VA_ARGS__)

void obdn_BuildBlas(Obdn_Memory* memory, const Obdn_Geometry* prim, AccelerationStructure* blas)
{
    VkBufferDeviceAddressInfo addrInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = prim->vertexRegion.buffer,
    };

    const VkDeviceAddress vertAddr = vkGetBufferDeviceAddress(memory->instance->device, &addrInfo) + prim->vertexRegion.offset;

    addrInfo.buffer = prim->indexRegion.buffer;
    
    const VkDeviceAddress indexAddr = vkGetBufferDeviceAddress(memory->instance->device, &addrInfo) + prim->indexRegion.offset;

    const VkAccelerationStructureGeometryTrianglesDataKHR triData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat  = OBDN_VERT_POS_FORMAT,
        .vertexStride  = sizeof(Vec3),
        .indexType     = OBDN_VERT_INDEX_TYPE,
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

    vkGetAccelerationStructureBuildSizesKHR(memory->instance->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildAS, &numTrianlges, &buildSizes); 

    blas->bufferRegion = obdn_RequestBufferRegion(memory, buildSizes.accelerationStructureSize, 
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
            OBDN_V_MEMORY_DEVICE_TYPE);

    const VkAccelerationStructureCreateInfoKHR accelStructInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = blas->bufferRegion.buffer,
        .offset = blas->bufferRegion.offset,
        .size   = blas->bufferRegion.size,
        .type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };

    V_ASSERT( vkCreateAccelerationStructureKHR(memory->instance->device, &accelStructInfo, NULL, &blas->handle) );

    //Obdn_V_BufferRegion scratchBufferRegion = obdn_v_RequestBufferRegion(buildSizes.buildScratchSize, 
    //        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
    //        OBDN_V_MEMORY_DEVICE_TYPE);


    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelStructProps = obdn_GetPhysicalDeviceAccelerationStructureProperties(memory->instance);

    Obdn_V_BufferRegion scratchBufferRegion = obdn_RequestBufferRegionAligned(memory, buildSizes.buildScratchSize, 
            accelStructProps.minAccelerationStructureScratchOffsetAlignment,
            OBDN_V_MEMORY_DEVICE_TYPE);

    buildAS.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildAS.dstAccelerationStructure = blas->handle;
    buildAS.scratchData.deviceAddress = obdn_GetBufferRegionAddress(&scratchBufferRegion);

    VkAccelerationStructureBuildRangeInfoKHR buildRange = {
        .firstVertex = 0,
        .primitiveCount = numTrianlges,
        .primitiveOffset = 0,
        .transformOffset = 0
    };

    const VkAccelerationStructureBuildRangeInfoKHR* ranges[1] = {&buildRange};

    Command cmd = obdn_CreateCommand(memory->instance, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_BeginCommandBuffer(cmd.buffer);

    vkCmdBuildAccelerationStructuresKHR(cmd.buffer, 1, &buildAS, ranges);

    obdn_EndCommandBuffer(cmd.buffer);

    hell_DPrint("HERE!\n");

    obdn_SubmitAndWait(&cmd, 0);

    obdn_DestroyCommand(cmd);

    obdn_FreeBufferRegion(&scratchBufferRegion);
}

void obdn_BuildTlas(Obdn_Memory* memory, const uint32_t count, const AccelerationStructure blasses[],
        const Coal_Mat4 xforms[],
        AccelerationStructure* tlas)
{
    //// Mat4 transform = m_Ident_Mat4();
    // instance transform member is a 4x3 row-major matrix
    // transform = m_Transpose_Mat4(&transform); // we don't need to because its identity, but leaving here to impress the point
    assert(xforms);
    assert(count > 0);

    VkAccelerationStructureInstanceKHR* instances = hell_Malloc(sizeof(VkAccelerationStructureInstanceKHR) * count);
    memset(instances, 0, sizeof(instances[0]) * count);
    for (int i = 0; i < count; i++)
    {
        Mat4 xformT = xforms[i];
        xformT = coal_Transpose_Mat4(xformT);
        VkTransformMatrixKHR transform;
        assert(sizeof(transform) == 12 * sizeof(float));
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                transform.matrix[i][j] = xformT.e[i][j]; 
            }
        }
        instances[i].accelerationStructureReference = obdn_GetBufferRegionAddress(&blasses[i].bufferRegion);
        instances[i].instanceCustomIndex = 0; // 0'th instance it
        instances[i].instanceShaderBindingTableRecordOffset = 0;
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[i].mask = 0xFF;
        instances[i].transform = transform;
    }


    BufferRegion instBuffer = obdn_RequestBufferRegion(memory, sizeof(*instances) * count,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            OBDN_V_MEMORY_HOST_GRAPHICS_TYPE);

    assert(instBuffer.hostData);
    memcpy(instBuffer.hostData, instances, sizeof(instances[0]) * count);
    hell_Free(instances);

    const VkAccelerationStructureGeometryInstancesDataKHR instanceData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .data.deviceAddress = obdn_GetBufferRegionAddress(&instBuffer),
        .arrayOfPointers = VK_FALSE
    };

    const VkAccelerationStructureGeometryKHR topAsGeometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometry.instances = instanceData,
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

    const uint32_t maxPrimCount = count; // if topAsInfo.geometryCount was > 1, this would be an array of counts

    VkAccelerationStructureBuildSizesInfoKHR buildSizes = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    vkGetAccelerationStructureBuildSizesKHR(memory->instance->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &topAsInfo, &maxPrimCount, &buildSizes); 

    tlas->bufferRegion = obdn_RequestBufferRegion(memory, buildSizes.accelerationStructureSize, 
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            OBDN_V_MEMORY_DEVICE_TYPE);

    const VkAccelerationStructureCreateInfoKHR asCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .buffer = tlas->bufferRegion.buffer,
        .offset = tlas->bufferRegion.offset,
        .size   = tlas->bufferRegion.size,
    };

    V_ASSERT( vkCreateAccelerationStructureKHR(memory->instance->device, &asCreateInfo, NULL, &tlas->handle) );

    // allocate and bind memory

    BufferRegion scratchBuffer = obdn_RequestBufferRegion(memory, buildSizes.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            OBDN_V_MEMORY_DEVICE_TYPE);

    topAsInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    topAsInfo.dstAccelerationStructure = tlas->handle;
    topAsInfo.scratchData.deviceAddress = obdn_GetBufferRegionAddress(&scratchBuffer);

    // so this is weird but ill break it down 
    // for each acceleration structure we want to build, we provide 
    // N buildRanges. This N is the same number as the geometryCount member of the
    // corresponding VkAccelerationStructureBuildGeometryInfoKHR. This means 
    // that each buildRange corresponds to a member of pGeometries in that struct.
    // the interpretation of the members of each buildRange depends on the 
    // VkAccelerationStructureGeometryKHR type that it ends up being paired with
    // For instance, firstVertex really only applies if that geometry type is triangles.
    // For instances, primitiveCount is the number or instances 
    const VkAccelerationStructureBuildRangeInfoKHR buildRange = {
        .firstVertex = 0,
        .primitiveCount = count, 
        .primitiveOffset = 0,
        .transformOffset = 0 };

    const VkAccelerationStructureBuildRangeInfoKHR* ranges[1] = {&buildRange};

    Command cmd = obdn_CreateCommand(memory->instance, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_BeginCommandBuffer(cmd.buffer);

    vkCmdBuildAccelerationStructuresKHR(cmd.buffer, 1, &topAsInfo, ranges);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitAndWait(&cmd, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_DestroyCommand(cmd);
    obdn_FreeBufferRegion(&scratchBuffer);
    obdn_FreeBufferRegion(&instBuffer);
}

void obdn_CreateShaderBindingTable(Obdn_Memory* memory, const uint32_t groupCount, const VkPipeline pipeline, ShaderBindingTable* sbt)
{
    memset(sbt, 0, sizeof(*sbt));

    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtprops = obdn_GetPhysicalDeviceRayTracingProperties(memory->instance);
    const uint32_t groupHandleSize = rtprops.shaderGroupHandleSize;
    const uint32_t baseAlignment = rtprops.shaderGroupBaseAlignment;
    const uint32_t sbtSize = groupCount * baseAlignment; // 3 shader groups: raygen, miss, closest hit

    void* shaderHandleData = hell_Malloc(sbtSize);

    DPRINT("ShaderGroup handle size: %d\n", groupHandleSize);
    DPRINT("ShaderGroup base alignment: %d\n", baseAlignment);
    DPRINT("ShaderGroups total size   : %d\n", sbtSize);

    VkResult r;
    r = vkGetRayTracingShaderGroupHandlesKHR(memory->instance->device, pipeline, 0, groupCount, sbtSize, shaderHandleData);
    assert( VK_SUCCESS == r );
    sbt->bufferRegion = obdn_RequestBufferRegionAligned(memory, sbtSize, baseAlignment, OBDN_V_MEMORY_HOST_GRAPHICS_TYPE);
    sbt->groupCount = groupCount;

    uint8_t* pSrc    = shaderHandleData;
    uint8_t* pTarget = sbt->bufferRegion.hostData;

    for (int i = 0; i < groupCount; i++) 
    {
        memcpy(pTarget, pSrc + i * groupHandleSize, groupHandleSize);
        pTarget += baseAlignment;
    }

    VkDeviceAddress bufferRegionAddress = obdn_GetBufferRegionAddress(&sbt->bufferRegion);
    sbt->raygenTable.deviceAddress = bufferRegionAddress;
    sbt->raygenTable.size          = baseAlignment;
    sbt->raygenTable.stride        = sbt->raygenTable.size; // must be this according to spec. stride not used for rgen.

    assert(groupHandleSize < baseAlignment); // for now.... 
    sbt->missTable.deviceAddress = bufferRegionAddress + baseAlignment;
    sbt->missTable.size          = baseAlignment;
    sbt->missTable.stride        = baseAlignment;

    sbt->hitTable.deviceAddress = bufferRegionAddress + baseAlignment * 2;
    sbt->hitTable.size          = baseAlignment;
    sbt->hitTable.stride        = baseAlignment;

    hell_Free(shaderHandleData);

    DPRINT("Created shader binding table\n");
}

void obdn_DestroyAccelerationStruct(VkDevice device, AccelerationStructure* as)
{
    vkDestroyAccelerationStructureKHR(device, as->handle, NULL);
    obdn_FreeBufferRegion(&as->bufferRegion);
    memset(as, 0, sizeof(*as));
}

void obdn_DestroyShaderBindingTable(Obdn_R_ShaderBindingTable* sb)
{
    obdn_FreeBufferRegion(&sb->bufferRegion);
    memset(sb, 0, sizeof(*sb));
}

