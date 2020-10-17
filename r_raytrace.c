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

VkAccelerationStructureKHR bottomLevelAS;
VkAccelerationStructureKHR topLevelAS;
static VkDeviceMemory  memoryBlas; //hacking this for now
static VkDeviceMemory  memoryTlas; //hacking this for now

static void allocObjectMemory(const VkAccelerationStructureKHR* accelStruct, VkDeviceMemory* memory)
{
    VkResult r;
    VkAccelerationStructureMemoryRequirementsInfoKHR memInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR,
        .accelerationStructure = *accelStruct,
        .buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        .type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR,
    };

    VkMemoryRequirements2 memReqs = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2
    };

    vkGetAccelerationStructureMemoryRequirementsKHR(device, &memInfo, &memReqs);

    printf("ACCEL STRUCT SIZE NEEDED: %ld\n", memReqs.memoryRequirements.size);

    VkMemoryAllocateInfo memAlloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.memoryRequirements.size,
        .memoryTypeIndex = tanto_v_GetMemoryType(
                memReqs.memoryRequirements.memoryTypeBits, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    r = vkAllocateMemory(device, &memAlloc, NULL, memory);
    assert( VK_SUCCESS == r );
}

static void createScratchBuffer(const VkAccelerationStructureKHR* accelStruct, ScratchBuffer* scratchBuffer)
{
    VkAccelerationStructureMemoryRequirementsInfoKHR memInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR,
        .accelerationStructure = *accelStruct,
        .buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        .type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR,
    };

    VkMemoryRequirements2 memReqs = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2
    };

    vkGetAccelerationStructureMemoryRequirementsKHR(device, &memInfo, &memReqs);

    scratchBuffer->size = memReqs.memoryRequirements.size;

    printf("Scratch buffer build size: %ld\n", scratchBuffer->size);

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | 
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .size = scratchBuffer->size,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
    };

    vkCreateBuffer(device, &bufferInfo, NULL, &scratchBuffer->buffer);

    VkMemoryRequirements bufferMemReqs;

    vkGetBufferMemoryRequirements(device, scratchBuffer->buffer, &bufferMemReqs);

    VkMemoryAllocateInfo memAlloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .memoryTypeIndex = tanto_v_GetMemoryType(bufferMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        .allocationSize = bufferMemReqs.size
    };

    vkAllocateMemory(device, &memAlloc, NULL, &scratchBuffer->memory);

    vkBindBufferMemory(device, scratchBuffer->buffer, scratchBuffer->memory, 0);

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

    VkResult r;
    r = vkCreateCommandPool(device, &cmdPoolCi, NULL, &rtCmdPool);
    assert( VK_SUCCESS == r );

    const VkCommandBufferAllocateInfo allocInfo = {
        .commandBufferCount = 1,
        .commandPool = rtCmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
    };

    r = vkAllocateCommandBuffers(device, &allocInfo, rtCmdBuffers);
    assert( VK_SUCCESS == r );
}

void tanto_r_BuildBlas(const Tanto_R_Mesh* mesh)
{
    VkResult r;

    const VkAccelerationStructureCreateGeometryTypeInfoKHR asCreate = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .indexType    = TANTO_VERT_INDEX_TYPE,
        .vertexFormat = TANTO_VERT_POS_FORMAT,
        .maxPrimitiveCount = mesh->indexCount / 3, // all triangles
        .maxVertexCount = mesh->vertexCount,
        .allowsTransforms = VK_FALSE // no adding transform matrices
    };

    const VkAccelerationStructureCreateInfoKHR accelStructInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .maxGeometryCount = 1,
        .type  = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .pGeometryInfos  = &asCreate
    };

    r = vkCreateAccelerationStructureKHR(device, &accelStructInfo, NULL, &bottomLevelAS);
    assert( VK_SUCCESS == r );

    allocObjectMemory(&bottomLevelAS, &memoryBlas);

    const VkBindAccelerationStructureMemoryInfoKHR bind = {
        .sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR,
        .accelerationStructure = bottomLevelAS,
        .memory = memoryBlas,
        .memoryOffset = 0,
    };

    r = vkBindAccelerationStructureMemoryKHR(device, 1, &bind);
    assert( VK_SUCCESS == r );

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

    // now allocating scratch memory for the build

    ScratchBuffer scratchBuffer;
    createScratchBuffer(&bottomLevelAS, &scratchBuffer);

    //VkQueryPool queryPool;
    //// create query pool
    //{
    //    VkQueryPoolCreateInfo info = {
    //        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
    //        .queryCount = 1,
    //        .queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
    //    };

    //    r = vkCreateQueryPool(device, &info, NULL, &queryPool);
    //    assert( VK_SUCCESS == r );
    //}

    const VkAccelerationStructureGeometryKHR* pGeometry = &asGeom;

    VkAccelerationStructureBuildGeometryInfoKHR buildAS = {
        .sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .update                    = VK_FALSE,
        .srcAccelerationStructure  = VK_NULL_HANDLE,
        .dstAccelerationStructure  = bottomLevelAS,
        .geometryArrayOfPointers   = VK_FALSE,
        .geometryCount             = 1,
        .scratchData.deviceAddress = scratchBuffer.address,
        .ppGeometries              = &pGeometry,
    };

    const VkAccelerationStructureBuildOffsetInfoKHR buildOffset = {
        .firstVertex = 0,
        .primitiveCount = asCreate.maxPrimitiveCount,
        .primitiveOffset = 0x0,
        .transformOffset = 0x0
    };

    const VkAccelerationStructureBuildOffsetInfoKHR* pBuildOffset = &buildOffset;

    VkCommandBufferBeginInfo cmdBegin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(rtCmdBuffers[0], &cmdBegin);

    vkCmdBuildAccelerationStructureKHR(rtCmdBuffers[0], 1, &buildAS, &pBuildOffset);

    vkEndCommandBuffer(rtCmdBuffers[0]);

    tanto_v_SubmitToQueueWait(&rtCmdBuffers[0], TANTO_V_QUEUE_GRAPHICS_TYPE, 0); // choosing the last queue for no reason

    vkResetCommandPool(device, rtCmdPool, 0);

    //vkDestroyQueryPool(device, queryPool, NULL);
    vkDestroyBuffer(device, scratchBuffer.buffer, NULL);
    vkFreeMemory(device, scratchBuffer.memory, NULL);
}

void tanto_r_BuildTlas(void)
{
    const VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

    const VkAccelerationStructureCreateGeometryTypeInfoKHR geometryCreate = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .maxPrimitiveCount = 1,
        .allowsTransforms = VK_FALSE
    };

    const VkAccelerationStructureCreateInfoKHR asCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = flags,
        .maxGeometryCount = 1,
        .pGeometryInfos = &geometryCreate,
    };

    VkResult r;
    r = vkCreateAccelerationStructureKHR(device, &asCreateInfo, NULL, &topLevelAS);
    assert( VK_SUCCESS == r );

    // allocate and bind memory

    allocObjectMemory(&topLevelAS, &memoryTlas);

    const VkBindAccelerationStructureMemoryInfoKHR bind = {
        .sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR,
        .accelerationStructure = topLevelAS,
        .memory = memoryTlas,
        .memoryOffset = 0,
    };

    r = vkBindAccelerationStructureMemoryKHR(device, 1, &bind);
    assert( VK_SUCCESS == r );

    ScratchBuffer scratchBuffer;

    createScratchBuffer(&topLevelAS, &scratchBuffer);

    VkAccelerationStructureDeviceAddressInfoKHR addrInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = bottomLevelAS,
    };

    const VkDeviceAddress blasAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addrInfo);

    VkTransformMatrixKHR transform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0
    };

    //Mat4 transform = m_Ident_Mat4();

    //// instance transform member is a 4x3 row-major matrix
    //transform = m_Transpose_Mat4(&transform); // we don't need to because its identity, but leaving here to impress the point

    VkAccelerationStructureInstanceKHR instance = {
        .accelerationStructureReference = blasAddress, // the 0'th blas
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
            .usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .size = 1 * sizeof(VkAccelerationStructureInstanceKHR),
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
        };

        r = vkCreateBuffer(device, &bufferInfo, NULL, &instBuffer);
        assert( VK_SUCCESS == r );

        VkMemoryRequirements memReqs;

        vkGetBufferMemoryRequirements(device, instBuffer, &memReqs);

        VkMemoryAllocateInfo memAlloc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .memoryTypeIndex = tanto_v_GetMemoryType(memReqs.memoryTypeBits, 
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT),
            .allocationSize = memReqs.size
        };

        r = vkAllocateMemory(device, &memAlloc, NULL, &instMemory);
        assert( VK_SUCCESS == r );

        vkBindBufferMemory(device, instBuffer, instMemory, 0);

        void* data;

        r = vkMapMemory(device, instMemory, 0, memReqs.size, 0, &data);
        assert( VK_SUCCESS == r );

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
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR
    };

    const VkAccelerationStructureGeometryKHR* pGeometry = &topAsGeometry;

    const VkAccelerationStructureBuildGeometryInfoKHR topAsInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .flags = flags,
        .update = VK_FALSE,
        .srcAccelerationStructure = VK_NULL_HANDLE,
        .dstAccelerationStructure = topLevelAS,
        .geometryArrayOfPointers = VK_FALSE,
        .scratchData = scratchBuffer.address,
        .geometryCount = 1,
        .ppGeometries = &pGeometry,
    };

    const VkAccelerationStructureBuildOffsetInfoKHR buildOffsetInfo = {
        .primitiveCount = 1,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0,
    };

    const VkCommandBufferBeginInfo cmdBegin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    const VkAccelerationStructureBuildOffsetInfoKHR* pBuildOffset = &buildOffsetInfo;

    r = vkBeginCommandBuffer(rtCmdBuffers[0], &cmdBegin);
    assert( VK_SUCCESS == r );

    vkCmdBuildAccelerationStructureKHR(rtCmdBuffers[0], 1, &topAsInfo, &pBuildOffset);

    vkEndCommandBuffer(rtCmdBuffers[0]);

    tanto_v_SubmitToQueueWait(&rtCmdBuffers[0], TANTO_V_QUEUE_GRAPHICS_TYPE, 0);

    vkDestroyBuffer(device, scratchBuffer.buffer, NULL);
    vkDestroyBuffer(device, instBuffer, NULL);
    vkFreeMemory(device, scratchBuffer.memory, NULL);
    vkFreeMemory(device, instMemory, NULL);
}

void tanto_r_InitRayTracing(void)
{
    VkPhysicalDeviceRayTracingPropertiesKHR rtProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR,
        .pNext = NULL
    };

    VkPhysicalDeviceProperties2 properties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rtProps
    };

    vkGetPhysicalDeviceProperties2(physicalDevice, &properties);

    printf("Max recursion depth: %d\n", rtProps.maxRecursionDepth);
    printf("Max instance count: %ld\n", rtProps.maxInstanceCount);

    initCmdPool();
}

void tanto_r_RayTraceDestroyAccelStructs(void)
{
    vkDestroyAccelerationStructureKHR(device, topLevelAS, NULL);
    vkDestroyAccelerationStructureKHR(device, bottomLevelAS, NULL);
    vkFreeMemory(device, memoryTlas, NULL);
    vkFreeMemory(device, memoryBlas, NULL);
}

void tanto_r_RayTraceCleanUp(void)
{
    tanto_r_RayTraceDestroyAccelStructs();
    vkDestroyCommandPool(device, rtCmdPool, NULL);
}
