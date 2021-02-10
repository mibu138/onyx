#include "v_loader.h"
#include "obsidian/t_def.h"
#include "v_def.h"
#include <assert.h>
#include <vulkan/vulkan_core.h>

static PFN_vkCreateAccelerationStructureKHR                pfn_vkCreateAccelerationStructureKHR;
static PFN_vkCmdBuildAccelerationStructuresKHR             pfn_vkCmdBuildAccelerationStructuresKHR;
static PFN_vkGetAccelerationStructureBuildSizesKHR         pfn_vkGetAccelerationStructureBuildSizesKHR;
static PFN_vkDestroyAccelerationStructureKHR               pfn_vkDestroyAccelerationStructureKHR;
static PFN_vkGetAccelerationStructureDeviceAddressKHR      pfn_vkGetAccelerationStructureDeviceAddressKHR;
static PFN_vkCreateRayTracingPipelinesKHR                  pfn_vkCreateRayTracingPipelinesKHR;
static PFN_vkGetRayTracingShaderGroupHandlesKHR            pfn_vkGetRayTracingShaderGroupHandlesKHR;
static PFN_vkCmdTraceRaysKHR                               pfn_vkCmdTraceRaysKHR;
static PFN_vkGetMemoryFdKHR                                pfn_vkGetMemoryFdKHR;
static PFN_vkGetSemaphoreFdKHR                             pfn_vkGetSemaphoreFdKHR;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(
        VkDevice device, 
        const VkAccelerationStructureCreateInfoKHR *pCreateInfo, 
        const VkAllocationCallbacks *pAllocator, 
        VkAccelerationStructureKHR *pAccelerationStructure)
{
    assert(pfn_vkCreateAccelerationStructureKHR);
    return pfn_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(
    VkDevice                                    device,
    VkAccelerationStructureBuildTypeKHR         buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
    const uint32_t*                             pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR*   pSizeInfo)
{
    assert(pfn_vkGetAccelerationStructureBuildSizesKHR);
    return pfn_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR(
        VkCommandBuffer commandBuffer, 
        uint32_t infoCount, 
        const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, 
        const VkAccelerationStructureBuildRangeInfoKHR *const *ppRangeInfos)
{
    assert(pfn_vkCmdBuildAccelerationStructuresKHR);
    return pfn_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppRangeInfos);
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
        VkDeferredOperationKHR deferredOperation,
        VkPipelineCache pipelineCache, 
        uint32_t createInfoCount, 
        const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, 
        const VkAllocationCallbacks *pAllocator, 
        VkPipeline *pPipelines)
{
    assert(pfn_vkCreateRayTracingPipelinesKHR);
    return pfn_vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
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

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryFdKHR(
    VkDevice                                    device,
    const VkMemoryGetFdInfoKHR*                 pGetFdInfo,
    int*                                        pFd)
{
    assert(pfn_vkGetMemoryFdKHR);
    return pfn_vkGetMemoryFdKHR(device, pGetFdInfo, pFd);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreFdKHR(
    VkDevice                                    device,
    const VkSemaphoreGetFdInfoKHR*              pGetFdInfo,
    int*                                        pFd)
{
    assert(pfn_vkGetSemaphoreFdKHR);
    return pfn_vkGetSemaphoreFdKHR(device, pGetFdInfo, pFd);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(
        VkCommandBuffer commandBuffer, 
        const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, 
        const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, 
        const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, 
        const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, 
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

void obdn_v_LoadFunctions(const VkDevice device)
{
    pfn_vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)
        vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
    pfn_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)
        vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
    pfn_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)
        vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
    pfn_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)
        vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"); 
    pfn_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)
        vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
    pfn_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)
        vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
    pfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)
        vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
    pfn_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)
        vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
    pfn_vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)
        vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    pfn_vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)
        vkGetDeviceProcAddr(device, "vkGetSemaphoreFdKHR");
    V1_PRINT("======= Vulkan Functions loaded ===== \n");
}
