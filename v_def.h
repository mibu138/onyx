#ifndef TANTO_V_DEF_H
#define TANTO_V_DEF_H

#include "v_vulkan.h"

#define TANTO_FRAME_COUNT 2
#define TANTO_G_QUEUE_COUNT 4

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(
        VkDevice device, 
        const VkAccelerationStructureCreateInfoKHR *pCreateInfo, 
        const VkAllocationCallbacks *pAllocator, 
        VkAccelerationStructureKHR *pAccelerationStructure);

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsKHR(
        VkDevice device, 
        const VkAccelerationStructureMemoryRequirementsInfoKHR *pInfo, 
        VkMemoryRequirements2 *pMemoryRequirements);

VKAPI_ATTR VkResult VKAPI_CALL vkBindAccelerationStructureMemoryKHR(
        VkDevice device, 
        uint32_t bindInfoCount, 
        const VkBindAccelerationStructureMemoryInfoKHR *pBindInfos);

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructureKHR(
        VkCommandBuffer commandBuffer, 
        uint32_t infoCount, 
        const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, 
        const VkAccelerationStructureBuildOffsetInfoKHR *const *ppOffsetInfos);

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(
        VkDevice device, 
        VkAccelerationStructureKHR accelerationStructure, 
        const VkAllocationCallbacks *pAllocator);

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(
        VkDevice device, 
        const VkAccelerationStructureDeviceAddressInfoKHR *pInfo);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(
        VkDevice device, 
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
        const VkStridedBufferRegionKHR *pRaygenShaderBindingTable, 
        const VkStridedBufferRegionKHR *pMissShaderBindingTable, 
        const VkStridedBufferRegionKHR *pHitShaderBindingTable, 
        const VkStridedBufferRegionKHR *pCallableShaderBindingTable, 
        uint32_t width, uint32_t height, uint32_t depth);

void tanto_v_LoadFunctions(const VkDevice* device);

#endif /* end of include guard: V_DEF_H */

