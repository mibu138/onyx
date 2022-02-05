#ifndef OBDN_VULKAN_H
#define OBDN_VULKAN_H

#ifdef UNIX
#define VK_USE_PLATFORM_XCB_KHR
#else
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <assert.h>
#include <hell/debug.h>
#include <hell/hell.h>
#include <string.h>
#include <vulkan/vulkan.h>

static inline void
obdn_VulkanErrorMessage(VkResult result, const char* filestr, int linenum,
                        const char* funcstr)
{
    if (result != VK_SUCCESS)
    {
        switch (result)
        {
#define CASE_PRINT(r)                                                          \
    case r:                                                                    \
        hell_Print("%s:%d:%s() :: ", filestr, linenum, funcstr);               \
        hell_Print(#r);                                                        \
        hell_Print("\n");                                                      \
        break
            CASE_PRINT(VK_NOT_READY);
            CASE_PRINT(VK_TIMEOUT);
            CASE_PRINT(VK_EVENT_SET);
            CASE_PRINT(VK_EVENT_RESET);
            CASE_PRINT(VK_INCOMPLETE);
            CASE_PRINT(VK_ERROR_OUT_OF_HOST_MEMORY);
            CASE_PRINT(VK_ERROR_OUT_OF_DEVICE_MEMORY);
            CASE_PRINT(VK_ERROR_INITIALIZATION_FAILED);
            CASE_PRINT(VK_ERROR_DEVICE_LOST);
            CASE_PRINT(VK_ERROR_MEMORY_MAP_FAILED);
            CASE_PRINT(VK_ERROR_LAYER_NOT_PRESENT);
            CASE_PRINT(VK_ERROR_EXTENSION_NOT_PRESENT);
            CASE_PRINT(VK_ERROR_FEATURE_NOT_PRESENT);
            CASE_PRINT(VK_ERROR_INCOMPATIBLE_DRIVER);
            CASE_PRINT(VK_ERROR_TOO_MANY_OBJECTS);
            CASE_PRINT(VK_ERROR_FORMAT_NOT_SUPPORTED);
            CASE_PRINT(VK_ERROR_FRAGMENTED_POOL);
            CASE_PRINT(VK_ERROR_UNKNOWN);
            CASE_PRINT(VK_ERROR_OUT_OF_POOL_MEMORY);
            CASE_PRINT(VK_ERROR_INVALID_EXTERNAL_HANDLE);
            CASE_PRINT(VK_ERROR_FRAGMENTATION);
            CASE_PRINT(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
            CASE_PRINT(VK_ERROR_SURFACE_LOST_KHR);
            CASE_PRINT(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
            CASE_PRINT(VK_SUBOPTIMAL_KHR);
            CASE_PRINT(VK_ERROR_OUT_OF_DATE_KHR);
            CASE_PRINT(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
            CASE_PRINT(VK_ERROR_VALIDATION_FAILED_EXT);
            CASE_PRINT(VK_ERROR_INVALID_SHADER_NV);
            CASE_PRINT(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
            CASE_PRINT(VK_ERROR_NOT_PERMITTED_EXT);
            CASE_PRINT(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
            CASE_PRINT(VK_THREAD_IDLE_KHR);
            CASE_PRINT(VK_THREAD_DONE_KHR);
            CASE_PRINT(VK_OPERATION_DEFERRED_KHR);
            CASE_PRINT(VK_OPERATION_NOT_DEFERRED_KHR);
            CASE_PRINT(VK_PIPELINE_COMPILE_REQUIRED_EXT);
#undef CASE_PRINT
        default:
            hell_DPrint("UNKNOWN ERROR");
        }
    }
    if (result < 0)
        assert(0);
}

// note VkResults >= 0 are considered success codes.
#ifndef NDEBUG
#define V_ASSERT(expr)                                                         \
    (obdn_VulkanErrorMessage(expr, __FILE__, __LINE__, __func__))
#else
#define V_ASSERT(expr) (expr)
#endif

#define OBDN_VK_DYNAMIC_STATE_CREATE_INFO()                                    \
    {                                                                          \
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO          \
    }
#define OBDN_VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO()                            \
    {                                                                          \
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO           \
    }
#define OBDN_VK_PIPELINE_LAYOUT_CREATE_INFO()                                  \
    {                                                                          \
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO                 \
    }
#define OBDN_VK_RENDER_PASS_BEGIN_INFO()                                       \
    {                                                                          \
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO                      \
    }
#define OBDN_VK_SUBMIT_INFO()                                                  \
    {                                                                          \
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO                                 \
    }
#define OBDN_VK_FENCE_CREATE_INFO()                                            \
    {                                                                          \
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO                           \
    }
#define OBDN_VK_IMAGE_MEMORY_BARRIER()                                         \
    {                                                                          \
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER                        \
    }

static inline VkDescriptorSetLayoutBinding
obdn_VkDescriptorSetLayoutBinding(uint32_t           binding,
                                  VkDescriptorType   descriptorType,
                                  uint32_t           descriptorCount,
                                  VkShaderStageFlags stageFlags,
                                  const VkSampler*   pImmutableSamplers)
{
    return (VkDescriptorSetLayoutBinding){.binding         = binding,
                                          .descriptorType  = descriptorType,
                                          .descriptorCount = descriptorCount,
                                          .stageFlags      = stageFlags,
                                          .pImmutableSamplers =
                                              pImmutableSamplers};
}

static inline VkShaderModuleCreateInfo
obdn_ShaderModuleCreateInfo(size_t codeSize, const void* pCode)
{
    return (VkShaderModuleCreateInfo){
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode    = (uint32_t*)pCode};
}

static inline VkPipelineShaderStageCreateInfo
obdn_PipelineShaderStageCreateInfo(
    VkShaderStageFlagBits stage, VkShaderModule module, const char* entryName,
    const VkSpecializationInfo* pSpecializationInfo /*can be NULL*/)
{
    return (VkPipelineShaderStageCreateInfo){
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = stage,
        .module = module,
        .pName  = entryName,
        .pSpecializationInfo = pSpecializationInfo};
}

static inline VkPipelineVertexInputStateCreateInfo
obdn_PipelineVertexInputStateCreateInfo(
    uint32_t                                 vertexBindingDescriptionCount,
    const VkVertexInputBindingDescription*   pVertexBindingDescriptions,
    uint32_t                                 vertexAttributeDescriptionCount,
    const VkVertexInputAttributeDescription* pVertexAttributeDescriptions)
{
    return (VkPipelineVertexInputStateCreateInfo){
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = vertexBindingDescriptionCount,
        .pVertexBindingDescriptions      = pVertexBindingDescriptions,
        .vertexAttributeDescriptionCount = vertexAttributeDescriptionCount,
        .pVertexAttributeDescriptions    = pVertexAttributeDescriptions};
}

static inline VkVertexInputBindingDescription
obdn_VertexInputBindingDescription(uint32_t binding, uint32_t stride,
                                   VkVertexInputRate inputRate)
{
    VkVertexInputBindingDescription d = {};
    d.binding                         = binding;
    d.stride                          = stride;
    d.inputRate                       = inputRate;
    return d;
}

static inline VkVertexInputAttributeDescription
obdn_VertexInputAttributeDescription(uint32_t location, uint32_t binding,
                                     VkFormat format, uint32_t offset)
{
    VkVertexInputAttributeDescription d = {};
    d.location                          = location;
    d.binding                           = binding;
    d.format                            = format;
    d.offset                            = offset;
    return d;
}

static inline VkPipelineInputAssemblyStateCreateInfo
obdn_PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology,
                                          VkBool32 primitiveRestartEnable)
{
    VkPipelineInputAssemblyStateCreateInfo c = {};
    c.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    c.topology = topology;
    c.primitiveRestartEnable = primitiveRestartEnable;
    return c;
}

static inline VkPipelineTessellationStateCreateInfo
obdn_PipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
{
    VkPipelineTessellationStateCreateInfo c = {};
    c.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    c.patchControlPoints = patchControlPoints;
    return c;
}

static inline VkPipelineViewportStateCreateInfo
obdn_PipelineViewportStateCreateInfo(uint32_t          viewportCount,
                                     const VkViewport* pViewports,
                                     uint32_t          scissorCount,
                                     const VkRect2D*   pScissors)
{
    VkPipelineViewportStateCreateInfo c = {};
    c.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    c.viewportCount = viewportCount;
    c.pViewports    = pViewports;
    c.scissorCount  = scissorCount;
    c.pScissors     = pScissors;
    return c;
}

static inline VkPipelineRasterizationStateCreateInfo
obdn_PipelineRasterizationStateCreateInfo(
    VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable,
    VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace,
    VkBool32 depthBiasEnable, float depthBiasConstantFactor,
    float depthBiasClamp, float depthBiasSlopeFactor, float lineWidth)
{
    VkPipelineRasterizationStateCreateInfo c = {};
    c.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    c.depthClampEnable        = depthClampEnable;
    c.rasterizerDiscardEnable = rasterizerDiscardEnable;
    c.polygonMode             = polygonMode;
    c.cullMode                = cullMode;
    c.frontFace               = frontFace;
    c.depthBiasEnable         = depthBiasEnable;
    c.depthBiasConstantFactor = depthBiasConstantFactor;
    c.depthBiasClamp          = depthBiasClamp;
    c.depthBiasSlopeFactor    = depthBiasSlopeFactor;
    c.lineWidth               = lineWidth;
    return c;
}

static inline VkPipelineMultisampleStateCreateInfo
obdn_PipelineMultisampleStateCreateInfo(
    VkSampleCountFlagBits rasterizationSamples, VkBool32 sampleShadingEnable,
    float minSampleShading, const VkSampleMask* pSampleMask,
    VkBool32 alphaToCoverageEnable, VkBool32 alphaToOneEnable)
{
    VkPipelineMultisampleStateCreateInfo c = {};
    c.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    c.rasterizationSamples  = rasterizationSamples;
    c.sampleShadingEnable   = sampleShadingEnable;
    c.minSampleShading      = minSampleShading;
    c.pSampleMask           = pSampleMask;
    c.alphaToCoverageEnable = alphaToCoverageEnable;
    c.alphaToOneEnable      = alphaToOneEnable;
    return c;
}

static inline VkPipelineDepthStencilStateCreateInfo
obdn_PipelineDepthStencilStateCreateInfo(
    VkBool32 depthTestEnable, VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable,
    VkBool32 stencilTestEnable, VkStencilOpState front, VkStencilOpState back,
    float minDepthBounds, float maxDepthBounds)
{
    VkPipelineDepthStencilStateCreateInfo c = {};
    c.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    c.depthTestEnable       = depthTestEnable;
    c.depthWriteEnable      = depthWriteEnable;
    c.depthCompareOp        = depthCompareOp;
    c.depthBoundsTestEnable = depthBoundsTestEnable;
    c.stencilTestEnable     = stencilTestEnable;
    c.front                 = front;
    c.back                  = back;
    c.minDepthBounds        = minDepthBounds;
    c.maxDepthBounds        = maxDepthBounds;
    return c;
}

static inline VkPipelineColorBlendStateCreateInfo
obdn_PipelineColorBlendStateCreateInfo(
    VkBool32 logicOpEnable, VkLogicOp logicOp, uint32_t attachmentCount,
    const VkPipelineColorBlendAttachmentState* pAttachments,
    float                                      blendConstants[4])
{
    VkPipelineColorBlendStateCreateInfo c = {};
    c.sType         = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    c.logicOpEnable = logicOpEnable;
    c.logicOp       = logicOp;
    c.attachmentCount = attachmentCount;
    c.pAttachments    = pAttachments;
    if (blendConstants)
        memcpy(c.blendConstants, blendConstants, sizeof(c.blendConstants));
    return c;
}

static inline VkPipelineColorBlendAttachmentState
obdn_PipelineColorBlendAttachmentState(
    VkBool32 blendEnable, VkBlendFactor srcColorBlendFactor,
    VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp,
    VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor,
    VkBlendOp alphaBlendOp, VkColorComponentFlags colorWriteMask)
{
    VkPipelineColorBlendAttachmentState s = {};
    s.blendEnable                         = blendEnable;
    s.srcColorBlendFactor                 = srcColorBlendFactor;
    s.dstColorBlendFactor                 = dstColorBlendFactor;
    s.colorBlendOp                        = colorBlendOp;
    s.srcAlphaBlendFactor                 = srcAlphaBlendFactor;
    s.dstAlphaBlendFactor                 = dstAlphaBlendFactor;
    s.alphaBlendOp                        = alphaBlendOp;
    s.colorWriteMask                      = colorWriteMask;
    return s;
}

static inline VkPipelineDynamicStateCreateInfo
obdn_PipelineDynamicStateCreateInfo(uint32_t              dynamicStateCount,
                                    const VkDynamicState* pDynamicStates)
{
    VkPipelineDynamicStateCreateInfo c = {};
    c.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    c.dynamicStateCount = dynamicStateCount;
    c.pDynamicStates    = pDynamicStates;
    return c;
}

static inline VkGraphicsPipelineCreateInfo
obdn_GraphicsPipelineCreateInfo(
    uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages,
    const VkPipelineVertexInputStateCreateInfo*   pVertexInputState,
    const VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState,
    const VkPipelineTessellationStateCreateInfo*  pTessellationState,
    const VkPipelineViewportStateCreateInfo*      pViewportState,
    const VkPipelineRasterizationStateCreateInfo* pRasterizationState,
    const VkPipelineMultisampleStateCreateInfo*   pMultisampleState,
    const VkPipelineDepthStencilStateCreateInfo*  pDepthStencilState,
    const VkPipelineColorBlendStateCreateInfo*    pColorBlendState,
    const VkPipelineDynamicStateCreateInfo*       pDynamicState,
    VkPipelineLayout layout, VkRenderPass renderPass, uint32_t subpass,
    VkPipeline basePipelineHandle, int32_t basePipelineIndex)
{
    VkGraphicsPipelineCreateInfo c = {};
    c.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    c.stageCount          = stageCount;
    c.pStages             = pStages;
    c.pVertexInputState   = pVertexInputState;
    c.pInputAssemblyState = pInputAssemblyState;
    c.pTessellationState  = pTessellationState;
    c.pViewportState      = pViewportState;
    c.pRasterizationState = pRasterizationState;
    c.pMultisampleState   = pMultisampleState;
    c.pDepthStencilState  = pDepthStencilState;
    c.pColorBlendState    = pColorBlendState;
    c.pDynamicState       = pDynamicState;
    c.layout              = layout;
    c.renderPass          = renderPass;
    c.subpass             = subpass;
    c.basePipelineHandle  = basePipelineHandle;
    c.basePipelineIndex   = basePipelineIndex;
    return c;
}

static inline VkAttachmentDescription
obdn_AttachmentDescription(VkFormat format, VkSampleCountFlagBits samples,
                           VkAttachmentLoadOp  loadOp,
                           VkAttachmentStoreOp storeOp,
                           VkAttachmentLoadOp  stencilLoadOp,
                           VkAttachmentStoreOp stencilStoreOp,
                           VkImageLayout       initialLayout,
                           VkImageLayout       finalLayout)
{
    VkAttachmentDescription c = {};
    c.format                  = format;
    c.samples                 = samples;
    c.loadOp                  = loadOp;
    c.storeOp                 = storeOp;
    c.stencilLoadOp           = stencilLoadOp;
    c.stencilStoreOp          = stencilStoreOp;
    c.initialLayout           = initialLayout;
    c.finalLayout             = finalLayout;
    return c;
}

static inline VkAttachmentReference
obdn_AttachmentReference(uint32_t attachment, VkImageLayout layout)
{
    VkAttachmentReference c = {};
    c.attachment            = attachment;
    c.layout                = layout;
    return c;
}

static inline VkSubpassDescription
obdn_SubpassDescription(VkPipelineBindPoint          pipelineBindPoint,
                        uint32_t                     inputAttachmentCount,
                        const VkAttachmentReference* pInputAttachments,
                        uint32_t                     colorAttachmentCount,
                        const VkAttachmentReference* pColorAttachments,
                        const VkAttachmentReference* pResolveAttachments,
                        const VkAttachmentReference* pDepthStencilAttachment,
                        uint32_t                     preserveAttachmentCount,
                        const uint32_t*              pPreserveAttachments)
{
    VkSubpassDescription c    = {};
    c.pipelineBindPoint       = pipelineBindPoint;
    c.inputAttachmentCount    = inputAttachmentCount;
    c.pInputAttachments       = pInputAttachments;
    c.colorAttachmentCount    = colorAttachmentCount;
    c.pColorAttachments       = pColorAttachments;
    c.pResolveAttachments     = pResolveAttachments;
    c.pDepthStencilAttachment = pDepthStencilAttachment;
    c.preserveAttachmentCount = preserveAttachmentCount;
    c.pPreserveAttachments    = pPreserveAttachments;
    return c;
}

static inline VkSubpassDependency
obdn_SubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass,
                       VkPipelineStageFlags srcStageMask,
                       VkPipelineStageFlags dstStageMask,
                       VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                       VkDependencyFlags dependencyFlags)
{
    VkSubpassDependency c = {};
    c.srcSubpass          = srcSubpass;
    c.dstSubpass          = dstSubpass;
    c.srcStageMask        = srcStageMask;
    c.dstStageMask        = dstStageMask;
    c.srcAccessMask       = srcAccessMask;
    c.dstAccessMask       = dstAccessMask;
    c.dependencyFlags     = dependencyFlags;
    return c;
}

static inline VkRenderPassCreateInfo
obdn_RenderPassCreateInfo(uint32_t                       attachmentCount,
                          const VkAttachmentDescription* pAttachments,
                          uint32_t                       subpassCount,
                          const VkSubpassDescription*    pSubpasses,
                          uint32_t                       dependencyCount,
                          const VkSubpassDependency*     pDependencies)
{
    VkRenderPassCreateInfo c = {};
    c.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    c.attachmentCount        = attachmentCount;
    c.pAttachments           = pAttachments;
    c.subpassCount           = subpassCount;
    c.pSubpasses             = pSubpasses;
    c.dependencyCount        = dependencyCount;
    c.pDependencies          = pDependencies;
    return c;
}

static inline VkRenderPassBeginInfo
obdn_RenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer framebuffer,
                         VkRect2D renderArea, uint32_t clearValueCount,
                         const VkClearValue* pClearValues)
{
    VkRenderPassBeginInfo c = {};
    c.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    c.renderPass            = renderPass;
    c.framebuffer           = framebuffer;
    c.renderArea            = renderArea;
    c.clearValueCount       = clearValueCount;
    c.pClearValues          = pClearValues;
    return c;
}

static inline VkDescriptorBufferInfo
obdn_DescriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset,
                          VkDeviceSize range)
{
    VkDescriptorBufferInfo c = {};
    c.buffer                 = buffer;
    c.offset                 = offset;
    c.range                  = range;
    return c;
}

static inline VkDescriptorImageInfo
obdn_DescriptorImageInfo(VkSampler sampler, VkImageView imageView,
                         VkImageLayout imageLayout)
{
    VkDescriptorImageInfo c = {};
    c.sampler               = sampler;
    c.imageView             = imageView;
    c.imageLayout           = imageLayout;
    return c;
}

static inline VkWriteDescriptorSet
obdn_WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding,
                        uint32_t dstArrayElement, uint32_t descriptorCount,
                        VkDescriptorType              descriptorType,
                        const VkDescriptorImageInfo*  pImageInfo,
                        const VkDescriptorBufferInfo* pBufferInfo,
                        const VkBufferView*           pTexelBufferView)
{
    VkWriteDescriptorSet c = {};
    c.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    c.dstSet               = dstSet;
    c.dstBinding           = dstBinding;
    c.dstArrayElement      = dstArrayElement;
    c.descriptorCount      = descriptorCount;
    c.descriptorType       = descriptorType;
    c.pImageInfo           = pImageInfo;
    c.pBufferInfo          = pBufferInfo;
    c.pTexelBufferView     = pTexelBufferView;
    return c;
}

static inline VkImageSubresourceRange
obdn_ImageSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel,
                           uint32_t levelCount, uint32_t baseArrayLayer,
                           uint32_t layerCount)
{
    VkImageSubresourceRange c = {};
    c.aspectMask              = aspectMask;
    c.baseMipLevel            = baseMipLevel;
    c.levelCount              = levelCount;
    c.baseArrayLayer          = baseArrayLayer;
    c.layerCount              = layerCount;
    return c;
}

static inline VkImageMemoryBarrier
obdn_ImageMemoryBarrier(VkAccessFlags srcAccessMask,
                        VkAccessFlags dstAccessMask, VkImageLayout oldLayout,
                        VkImageLayout newLayout, uint32_t srcQueueFamilyIndex,
                        uint32_t dstQueueFamilyIndex, VkImage image,
                        VkImageSubresourceRange subresourceRange)
{
    VkImageMemoryBarrier c = {};
    c.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    c.srcAccessMask        = srcAccessMask;
    c.dstAccessMask        = dstAccessMask;
    c.oldLayout            = oldLayout;
    c.newLayout            = newLayout;
    c.srcQueueFamilyIndex  = srcQueueFamilyIndex;
    c.dstQueueFamilyIndex  = dstQueueFamilyIndex;
    c.image                = image;
    c.subresourceRange     = subresourceRange;
    return c;
}

static inline VkMemoryBarrier
obdn_MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
    VkMemoryBarrier c = {};
    c.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    c.pNext           = NULL;
    c.srcAccessMask   = srcAccessMask;
    c.dstAccessMask   = dstAccessMask;
    return c;
}

static inline VkSubmitInfo
obdn_SubmitInfo(uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores,
                const VkPipelineStageFlags* pWaitDstStageMask,
                uint32_t                    commandBufferCount,
                const VkCommandBuffer*      pCommandBuffers,
                uint32_t                    signalSemaphoreCount,
                const VkSemaphore*          pSignalSemaphores)
{
    VkSubmitInfo c         = {};
    c.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    c.waitSemaphoreCount   = waitSemaphoreCount;
    c.pWaitSemaphores      = pWaitSemaphores;
    c.pWaitDstStageMask    = pWaitDstStageMask;
    c.commandBufferCount   = commandBufferCount;
    c.pCommandBuffers      = pCommandBuffers;
    c.signalSemaphoreCount = signalSemaphoreCount;
    c.pSignalSemaphores    = pSignalSemaphores;
    return c;
}

static inline VkCommandBufferSubmitInfo
obdn_CommandBufferSubmitInfo(VkCommandBuffer commandBuffer, uint32_t deviceMask)
{
    VkCommandBufferSubmitInfo c = {};
    c.sType                     = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    c.pNext                     = NULL;
    c.commandBuffer             = commandBuffer;
    c.deviceMask                = deviceMask;
    return c;
}

static inline VkSemaphoreSubmitInfo
obdn_SemaphoreSubmitInfo(
    VkSemaphore              semaphore,
    uint64_t                 value,
    VkPipelineStageFlags2    stageMask,
    uint32_t                 deviceIndex
        )
{
    VkSemaphoreSubmitInfo c = {};
    c.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    c.pNext = NULL;
    c.semaphore = semaphore;
    c.value = value;
    c.stageMask = stageMask;
    c.deviceIndex = deviceIndex;
    return c;
}

static inline VkSubmitInfo2
obdn_SubmitInfo2(VkSubmitFlags flags, uint32_t waitSemaphoreInfoCount,
                 const VkSemaphoreSubmitInfo*     pWaitSemaphoreInfos,
                 uint32_t                         commandBufferInfoCount,
                 const VkCommandBufferSubmitInfo* pCommandBufferInfos,
                 uint32_t                         signalSemaphoreInfoCount,
                 const VkSemaphoreSubmitInfo*     pSignalSemaphoreInfos)
{
    VkSubmitInfo2 c            = {};
    c.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    c.pNext                    = NULL;
    c.flags                    = flags;
    c.waitSemaphoreInfoCount   = waitSemaphoreInfoCount;
    c.pWaitSemaphoreInfos      = pWaitSemaphoreInfos;
    c.commandBufferInfoCount   = commandBufferInfoCount;
    c.pCommandBufferInfos      = pCommandBufferInfos;
    c.signalSemaphoreInfoCount = signalSemaphoreInfoCount;
    c.pSignalSemaphoreInfos    = pSignalSemaphoreInfos;
    return c;
}

#endif /* end of include guard: OBDN_VULKAN_H */
