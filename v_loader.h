#ifndef OBDN_V_LOADER_H
#define OBDN_V_LOADER_H

#include "v_vulkan.h"

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(
    VkDevice                                    device,
    VkAccelerationStructureBuildTypeKHR         buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
    const uint32_t*                             pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR*   pSizeInfo);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(
        VkDevice device, 
        const VkAccelerationStructureCreateInfoKHR *pCreateInfo, 
        const VkAllocationCallbacks *pAllocator, 
        VkAccelerationStructureKHR *pAccelerationStructure);

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructureKHR(
        VkCommandBuffer commandBuffer, 
        uint32_t infoCount, 
        const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, 
        const VkAccelerationStructureBuildRangeInfoKHR* const *ppRangeInfos);

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(
        VkDevice device, 
        VkAccelerationStructureKHR accelerationStructure, 
        const VkAllocationCallbacks *pAllocator);

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(
        VkDevice device, 
        const VkAccelerationStructureDeviceAddressInfoKHR *pInfo);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(
        VkDevice device, 
        VkDeferredOperationKHR deferredOperation,
        VkPipelineCache pipelineCache, 
        uint32_t createInfoCount, 
        const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, 
        const VkAllocationCallbacks *pAllocator, 
        VkPipeline *pPipelines);

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(
        VkDevice device, 
        VkPipeline pipeline, 
        uint32_t firstGroup, 
        uint32_t groupCount, 
        size_t dataSize, 
        void *pData);

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(
        VkCommandBuffer commandBuffer, 
        const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, 
        const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, 
        const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, 
        const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, 
        uint32_t width, uint32_t height, uint32_t depth);

void obdn_v_LoadFunctions(const VkDevice* device);

#endif /* end of include guard: OBDN_V_LOADER_H */

