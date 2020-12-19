#include "r_raytrace.h"
#include "v_video.h"
#include "r_render.h"
#include "v_memory.h"
#include "t_utils.h"
#include <assert.h>
#include "t_def.h"
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_core.h>

static VkCommandPool   rtCmdPool;
static VkCommandBuffer rtCmdBuffers[1];

typedef struct {
    VkBuffer        buffer;
    VkDeviceSize    size;
    VkDeviceMemory  memory;
    VkDeviceAddress address;
} ScratchBuffer;

AccelerationStructure bottomLevelAS;
AccelerationStructure topLevelAS;

static void createAccelerationStructureBuffer(AccelerationStructure* accelStruct, 
        const VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
    VkBufferCreateInfo bufferCreate = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = buildSizeInfo.accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    };

    V_ASSERT( vkCreateBuffer(device, &bufferCreate, NULL, &accelStruct->buffer) );

    VkMemoryRequirements memReqs;

    vkGetBufferMemoryRequirements(device, accelStruct->buffer, &memReqs);

    printf("ACCEL STRUCT SIZE NEEDED: %ld\n", memReqs.size);

    VkMemoryAllocateFlagsInfo allocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    };

    VkMemoryAllocateInfo memAlloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = tanto_v_GetMemoryType(
                memReqs.memoryTypeBits, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        .pNext = &allocFlagsInfo
    };

    V_ASSERT( vkAllocateMemory(device, &memAlloc, NULL, &accelStruct->memory) );
    V_ASSERT( vkBindBufferMemory(device, accelStruct->buffer, accelStruct->memory, 0) );

    accelStruct->size = buildSizeInfo.accelerationStructureSize;
}

static void createScratchBuffer(VkDeviceSize size, ScratchBuffer* scratchBuffer)
{
    VkBufferCreateInfo bufferCreate = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    };

    V_ASSERT( vkCreateBuffer(device, &bufferCreate, NULL, &scratchBuffer->buffer) );

    VkMemoryRequirements memReqs;

    vkGetBufferMemoryRequirements(device, scratchBuffer->buffer, &memReqs);

    VkMemoryAllocateFlagsInfo allocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    };

    VkMemoryAllocateInfo memAlloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = tanto_v_GetMemoryType(
                memReqs.memoryTypeBits, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        .pNext = &allocFlagsInfo
    };

    scratchBuffer->size = memReqs.size;

    printf("Scratch buffer build size: %ld\n", scratchBuffer->size);

    V_ASSERT( vkAllocateMemory(device, &memAlloc, NULL, &scratchBuffer->memory) );

    V_ASSERT( vkBindBufferMemory(device, scratchBuffer->buffer, scratchBuffer->memory, 0) );

    VkBufferDeviceAddressInfo addrInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = scratchBuffer->buffer,
    };

    scratchBuffer->address = vkGetBufferDeviceAddress(device, &addrInfo);
}

static void initCmdPool(void)
{
    const VkCommandPoolCreateInfo cmdPoolCi = {
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };

    V_ASSERT( vkCreateCommandPool(device, &cmdPoolCi, NULL, &rtCmdPool) );

    const VkCommandBufferAllocateInfo allocInfo = {
        .commandBufferCount = 1,
        .commandPool = rtCmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
    };

    V_ASSERT( vkAllocateCommandBuffers(device, &allocInfo, rtCmdBuffers) );
}

void tanto_r_BuildBlas(const Tanto_R_Mesh* mesh)
{
    VkBufferDeviceAddressInfo addrInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = mesh->vertexBlock.buffer,
    };

    const VkDeviceAddress vertAddr  = vkGetBufferDeviceAddress(device, &addrInfo) + mesh->vertexBlock.offset;

    addrInfo.buffer = mesh->indexBlock.buffer;
    
    const VkDeviceAddress indexAddr = vkGetBufferDeviceAddress(device, &addrInfo) + mesh->indexBlock.offset;

    const VkAccelerationStructureGeometryTrianglesDataKHR triData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat  = TANTO_VERT_POS_FORMAT,
        .vertexStride  = sizeof(Tanto_R_Attribute),
        .indexType     = TANTO_VERT_INDEX_TYPE,
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

    const uint32_t numTrianlges = mesh->indexCount / 3;

    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildAS, &numTrianlges, &buildSizes); 

    createAccelerationStructureBuffer(&bottomLevelAS, buildSizes); 

    const VkAccelerationStructureCreateInfoKHR accelStructInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = bottomLevelAS.buffer,
        .size   = bottomLevelAS.size,
        .type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };

    V_ASSERT( vkCreateAccelerationStructureKHR(device, &accelStructInfo, NULL, &bottomLevelAS.handle) );

    ScratchBuffer scratchBuffer;

    createScratchBuffer(buildSizes.buildScratchSize, &scratchBuffer);

    buildAS.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildAS.dstAccelerationStructure = bottomLevelAS.handle;
    buildAS.scratchData.deviceAddress = scratchBuffer.address;

    VkAccelerationStructureBuildRangeInfoKHR buildRange = {
        .firstVertex = 0,
        .primitiveCount = numTrianlges,
        .primitiveOffset = 0,
        .transformOffset = 0
    };

    const VkAccelerationStructureBuildRangeInfoKHR* ranges[1] = {&buildRange};

    VkCommandBufferBeginInfo cmdBegin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(rtCmdBuffers[0], &cmdBegin);

    vkCmdBuildAccelerationStructuresKHR(rtCmdBuffers[0], 1, &buildAS, ranges);

    vkEndCommandBuffer(rtCmdBuffers[0]);

    tanto_v_SubmitToQueueWait(&rtCmdBuffers[0], TANTO_V_QUEUE_GRAPHICS_TYPE, 0); // choosing the last queue for no reason

    vkResetCommandPool(device, rtCmdPool, 0);

    //vkDestroyQueryPool(device, queryPool, NULL);
    vkDestroyBuffer(device, scratchBuffer.buffer, NULL);
    vkFreeMemory(device, scratchBuffer.memory, NULL);

    VkAccelerationStructureDeviceAddressInfoKHR blasAddrInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = bottomLevelAS.handle
    };

    bottomLevelAS.address = vkGetAccelerationStructureDeviceAddressKHR(device, &blasAddrInfo);
}

void tanto_r_BuildTlas(void)
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
        .accelerationStructureReference = bottomLevelAS.address, // the 0'th blas
        .instanceCustomIndex = 0, // 0'th instance it
        .instanceShaderBindingTableRecordOffset = 0, 
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .mask = 0xFF,
        .transform = transform
    };

    VkBuffer instBuffer;
    VkDeviceMemory instMemory; 
    VkDeviceAddress instAddress;

    {
        VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .size = 1 * sizeof(VkAccelerationStructureInstanceKHR),
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
        };

        V_ASSERT( vkCreateBuffer(device, &bufferInfo, NULL, &instBuffer) );

        VkMemoryRequirements memReqs;

        vkGetBufferMemoryRequirements(device, instBuffer, &memReqs);

        VkMemoryAllocateInfo memAlloc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .memoryTypeIndex = tanto_v_GetMemoryType(memReqs.memoryTypeBits, 
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            .allocationSize = memReqs.size
        };

        V_ASSERT( vkAllocateMemory(device, &memAlloc, NULL, &instMemory) );

        vkBindBufferMemory(device, instBuffer, instMemory, 0);

        void* data;

        V_ASSERT( vkMapMemory(device, instMemory, 0, memReqs.size, 0, &data) );

        memcpy(data, &instance, 1 * sizeof(instance));

        vkUnmapMemory(device, instMemory);

        VkBufferDeviceAddressInfo addrInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = instBuffer
        };

        instAddress = vkGetBufferDeviceAddress(device, &addrInfo);
    }

    const VkAccelerationStructureGeometryInstancesDataKHR instanceData = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .data.deviceAddress = instAddress,
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

    createAccelerationStructureBuffer(&topLevelAS, buildSizes);

    const VkAccelerationStructureCreateInfoKHR asCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .buffer = topLevelAS.buffer,
        .size   = topLevelAS.size,
    };

    V_ASSERT( vkCreateAccelerationStructureKHR(device, &asCreateInfo, NULL, &topLevelAS.handle) );

    // allocate and bind memory

    ScratchBuffer scratchBuffer;

    createScratchBuffer(buildSizes.buildScratchSize, &scratchBuffer);

    topAsInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    topAsInfo.dstAccelerationStructure = topLevelAS.handle;
    topAsInfo.scratchData.deviceAddress = scratchBuffer.address;

    const VkAccelerationStructureBuildRangeInfoKHR buildRange = {
        .firstVertex = 0,
        .primitiveCount = 1,
        .primitiveOffset = 0,
        .transformOffset = 0
    };

    const VkAccelerationStructureBuildRangeInfoKHR* ranges[1] = {&buildRange};

    const VkCommandBufferBeginInfo cmdBegin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    V_ASSERT( vkBeginCommandBuffer(rtCmdBuffers[0], &cmdBegin) );

    vkCmdBuildAccelerationStructuresKHR(rtCmdBuffers[0], 1, &topAsInfo, ranges);

    vkEndCommandBuffer(rtCmdBuffers[0]);

    tanto_v_SubmitToQueueWait(&rtCmdBuffers[0], TANTO_V_QUEUE_GRAPHICS_TYPE, 0);

    vkDestroyBuffer(device, scratchBuffer.buffer, NULL);
    vkDestroyBuffer(device, instBuffer, NULL);
    vkFreeMemory(device, scratchBuffer.memory, NULL);
    vkFreeMemory(device, instMemory, NULL);

    VkAccelerationStructureDeviceAddressInfoKHR blasAddrInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = topLevelAS.handle
    };

    bottomLevelAS.address = vkGetAccelerationStructureDeviceAddressKHR(device, &blasAddrInfo);
}

void tanto_r_InitRayTracing(void)
{
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
        .pNext = NULL
    };

    VkPhysicalDeviceProperties2 properties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rtProps
    };

    vkGetPhysicalDeviceProperties2(physicalDevice, &properties);

    printf("Max recursion depth: %d\n", rtProps.maxRayRecursionDepth);

    initCmdPool();
}

void tanto_r_RayTraceDestroyAccelStructs(void)
{
    vkDestroyAccelerationStructureKHR(device, topLevelAS.handle, NULL);
    vkDestroyAccelerationStructureKHR(device, bottomLevelAS.handle, NULL);
    vkDestroyBuffer(device, topLevelAS.buffer, NULL);
    vkDestroyBuffer(device, bottomLevelAS.buffer, NULL);
    vkFreeMemory(device, topLevelAS.memory, NULL);
    vkFreeMemory(device, bottomLevelAS.memory, NULL);
}

void tanto_r_RayTraceCleanUp(void)
{
    tanto_r_RayTraceDestroyAccelStructs();
    vkDestroyCommandPool(device, rtCmdPool, NULL);
}
