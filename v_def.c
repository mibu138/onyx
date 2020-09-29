#include "v_def.h"
#include <assert.h>
#include <vulkan/vulkan_core.h>

static PFN_vkCreateAccelerationStructureKHR                pfn_vkCreateAccelerationStructureKHR;
static PFN_vkGetAccelerationStructureMemoryRequirementsKHR pfn_vkGetAccelerationStructureMemoryRequirementsKHR;
static PFN_vkBindAccelerationStructureMemoryKHR            pfn_vkBindAccelerationStructureMemoryKHR;
static PFN_vkCmdBuildAccelerationStructureKHR              pfn_vkCmdBuildAccelerationStructureKHR;
static PFN_vkDestroyAccelerationStructureKHR               pfn_vkDestroyAccelerationStructureKHR;
static PFN_vkGetAccelerationStructureDeviceAddressKHR      pfn_vkGetAccelerationStructureDeviceAddressKHR;
static PFN_vkCreateRayTracingPipelinesKHR                  pfn_vkCreateRayTracingPipelinesKHR;
static PFN_vkGetRayTracingShaderGroupHandlesKHR            pfn_vkGetRayTracingShaderGroupHandlesKHR;
static PFN_vkCmdTraceRaysKHR                               pfn_vkCmdTraceRaysKHR;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(
        VkDevice device, 
        const VkAccelerationStructureCreateInfoKHR *pCreateInfo, 
        const VkAllocationCallbacks *pAllocator, 
        VkAccelerationStructureKHR *pAccelerationStructure)
{
    assert(pfn_vkCreateAccelerationStructureKHR);
    return pfn_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsKHR(
        VkDevice device, 
        const VkAccelerationStructureMemoryRequirementsInfoKHR *pInfo, 
        VkMemoryRequirements2 *pMemoryRequirements)
{
    assert(pfn_vkGetAccelerationStructureMemoryRequirementsKHR);
    return pfn_vkGetAccelerationStructureMemoryRequirementsKHR(device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindAccelerationStructureMemoryKHR(
        VkDevice device, 
        uint32_t bindInfoCount, 
        const VkBindAccelerationStructureMemoryInfoKHR *pBindInfos)
{
    assert(pfn_vkBindAccelerationStructureMemoryKHR);
    return pfn_vkBindAccelerationStructureMemoryKHR(device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructureKHR(
        VkCommandBuffer commandBuffer, 
        uint32_t infoCount, 
        const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, 
        const VkAccelerationStructureBuildOffsetInfoKHR *const *ppOffsetInfos)
{
    assert(pfn_vkCmdBuildAccelerationStructureKHR);
    return pfn_vkCmdBuildAccelerationStructureKHR(commandBuffer, infoCount, pInfos, ppOffsetInfos);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(
        VkDevice device, 
        VkAccelerationStructureKHR accelerationStructure, 
        const VkAllocationCallbacks *pAllocator)
{
    assert(pfn_vkDestroyAccelerationStructureKHR);
    return pfn_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(
        VkDevice device, 
        const VkAccelerationStructureDeviceAddressInfoKHR *pInfo)
{
    assert(pfn_vkGetAccelerationStructureDeviceAddressKHR);
    return pfn_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(
        VkDevice device, 
        VkPipelineCache pipelineCache, 
        uint32_t createInfoCount, 
        const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, 
        const VkAllocationCallbacks *pAllocator, 
        VkPipeline *pPipelines)
{
    assert(pfn_vkCreateRayTracingPipelinesKHR);
    return pfn_vkCreateRayTracingPipelinesKHR(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(
        VkDevice device, 
        VkPipeline pipeline, 
        uint32_t firstGroup, 
        uint32_t groupCount, 
        size_t dataSize, 
        void *pData)
{
    assert(pfn_vkGetRayTracingShaderGroupHandlesKHR);
    return pfn_vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(
        VkCommandBuffer commandBuffer, 
        const VkStridedBufferRegionKHR *pRaygenShaderBindingTable, 
        const VkStridedBufferRegionKHR *pMissShaderBindingTable, 
        const VkStridedBufferRegionKHR *pHitShaderBindingTable, 
        const VkStridedBufferRegionKHR *pCallableShaderBindingTable, 
        uint32_t width, uint32_t height, uint32_t depth)
{
    assert(pfn_vkCmdTraceRaysKHR);
    return pfn_vkCmdTraceRaysKHR(commandBuffer,
            pRaygenShaderBindingTable,
            pMissShaderBindingTable,
            pHitShaderBindingTable,
            pCallableShaderBindingTable,
            width, height, depth);
}

void v_LoadFunctions(const VkDevice* device)
{
    pfn_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)
        vkGetDeviceProcAddr(*device, "vkCreateAccelerationStructureKHR");
    pfn_vkGetAccelerationStructureMemoryRequirementsKHR = (PFN_vkGetAccelerationStructureMemoryRequirementsKHR)
        vkGetDeviceProcAddr(*device, "vkGetAccelerationStructureMemoryRequirementsKHR");
    pfn_vkBindAccelerationStructureMemoryKHR = (PFN_vkBindAccelerationStructureMemoryKHR)
        vkGetDeviceProcAddr(*device, "vkBindAccelerationStructureMemoryKHR");
    pfn_vkCmdBuildAccelerationStructureKHR = (PFN_vkCmdBuildAccelerationStructureKHR)
        vkGetDeviceProcAddr(*device, "vkCmdBuildAccelerationStructureKHR");
    pfn_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)
        vkGetDeviceProcAddr(*device, "vkDestroyAccelerationStructureKHR"); 
    pfn_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)
        vkGetDeviceProcAddr(*device, "vkGetAccelerationStructureDeviceAddressKHR");
    pfn_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)
        vkGetDeviceProcAddr(*device, "vkCreateRayTracingPipelinesKHR");
    pfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)
        vkGetDeviceProcAddr(*device, "vkGetRayTracingShaderGroupHandlesKHR");
    pfn_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)
        vkGetDeviceProcAddr(*device, "vkCmdTraceRaysKHR");
}
