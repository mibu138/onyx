#include "command.h"
#include "video.h"
#include "private.h"
#include <string.h>

void onyx_SubmitAndWait(Onyx_Command* cmd, const uint32_t queueIndex)
{
    onyx_SubmitToQueueWait(cmd->instance, &cmd->buffer, cmd->queueFamily, queueIndex);
}

Onyx_Command onyx_CreateCommand(const Onyx_Instance* instance, const Onyx_V_QueueType queueFamilyType)
{
    Onyx_Command cmd = {
        .queueFamily = onyx_GetQueueFamilyIndex(instance, queueFamilyType),
        .instance = instance
    };

    const VkCommandPoolCreateInfo cmdPoolCi = {
        .queueFamilyIndex = cmd.queueFamily,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };

    V_ASSERT( vkCreateCommandPool(instance->device, &cmdPoolCi, NULL, &cmd.pool) );

    const VkCommandBufferAllocateInfo allocInfo = {
        .commandBufferCount = 1,
        .commandPool = cmd.pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
    };

    V_ASSERT( vkAllocateCommandBuffers(instance->device, &allocInfo, &cmd.buffer) );

    const VkSemaphoreCreateInfo semaCi = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    V_ASSERT( vkCreateSemaphore(instance->device, &semaCi, NULL, &cmd.semaphore) );

    const VkFenceCreateInfo fenceCi = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    V_ASSERT( vkCreateFence(instance->device, &fenceCi, NULL, &cmd.fence) );

    return cmd;
}

Onyx_CommandPool onyx_CreateCommandPool(VkDevice device,
    uint32_t queueFamilyIndex,
    VkCommandPoolCreateFlags poolflags,
    uint32_t bufcount)
{
    Onyx_CommandPool pool = {};
    pool.queueFamily = queueFamilyIndex;
    const VkCommandPoolCreateInfo cmdPoolCi = {
        .queueFamilyIndex = pool.queueFamily,
        .flags = poolflags,
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };

    V_ASSERT( vkCreateCommandPool(device, &cmdPoolCi, NULL, &pool.pool) );

    const VkCommandBufferAllocateInfo ai = {
        .commandBufferCount = bufcount,
        .commandPool = pool.pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
    };

    pool.cmdbufs = hell_Malloc(sizeof(VkCommandBuffer) * bufcount);

    V_ASSERT( vkAllocateCommandBuffers(device, &ai, pool.cmdbufs) );

    pool.cmdbuf_count = bufcount;

    return pool;
}

void onyx_DestroyCommandPool(VkDevice device, Onyx_CommandPool* pool)
{
    vkDestroyCommandPool(device, pool->pool, NULL);
    hell_Free(pool->cmdbufs);
    memset(pool, 0, sizeof(*pool));
}

void onyx_BeginCommandBuffer(VkCommandBuffer cmdBuf)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    V_ASSERT( vkBeginCommandBuffer(cmdBuf, &beginInfo) );
}

void onyx_BeginCommandBufferOneTimeSubmit(VkCommandBuffer cmdBuf)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    V_ASSERT( vkBeginCommandBuffer(cmdBuf, &beginInfo) );
}

void onyx_EndCommandBuffer(VkCommandBuffer cmdBuf)
{
    vkEndCommandBuffer(cmdBuf);
}


void onyx_WaitForFence(VkDevice device, VkFence* fence)
{
    V_ASSERT(vkWaitForFences(device, 1, fence, VK_TRUE, UINT64_MAX));
    V_ASSERT(vkResetFences(device, 1, fence));
}

void onyx_WaitForFence_TimeOut(VkDevice device, VkFence* fence, uint64_t timeout_ns)
{
    V_ASSERT(vkWaitForFences(device, 1, fence, VK_TRUE, timeout_ns));
    V_ASSERT(vkResetFences(device, 1, fence));
}


void onyx_DestroyFence(VkDevice device, VkFence fence)
{
    vkDestroyFence(device, fence, NULL);
}

void onyx_DestroySemaphore(VkDevice device, VkSemaphore semaphore)
{
    vkDestroySemaphore(device, semaphore, NULL);
}

void onyx_WaitForFenceNoReset(VkDevice device, VkFence* fence)
{
    vkWaitForFences(device, 1, fence, VK_TRUE, UINT64_MAX);
}

void onyx_DestroyCommand(Onyx_Command cmd)
{
    vkDestroyCommandPool(cmd.instance->device, cmd.pool, NULL);
    vkDestroyFence(cmd.instance->device, cmd.fence, NULL);
    vkDestroySemaphore(cmd.instance->device, cmd.semaphore, NULL);
}

void onyx_ResetCommand(Onyx_Command* cmd)
{
    vkResetCommandPool(cmd->instance->device, cmd->pool, 0);
}

void onyx_v_MemoryBarrier(
    VkCommandBuffer      commandBuffer,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkDependencyFlags    dependencyFlags,
    VkAccessFlags        srcAccessMask,
    VkAccessFlags        dstAccessMask)
{
    const VkMemoryBarrier barrier = {
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext         = NULL,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask 
    };

    vkCmdPipelineBarrier(commandBuffer, srcStageMask, 
            dstStageMask, dependencyFlags, 
            1, &barrier, 0, NULL, 0, NULL);
}

void onyx_CreateFence(VkDevice device, VkFence* fence)
{
    VkFenceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    V_ASSERT( vkCreateFence(device, &ci, NULL, fence) );
}

void onyx_CreateSemaphore(VkDevice device, VkSemaphore* semaphore)
{
    const VkSemaphoreCreateInfo semaCi = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    V_ASSERT( vkCreateSemaphore(device, &semaCi, NULL, semaphore) );
}

void onyx_CreateSemaphores(VkDevice device, u32 count, VkSemaphore* semas)
{
    const VkSemaphoreCreateInfo semaCi = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    for (u32 i = 0; i < count; ++i) 
    {
        V_ASSERT( vkCreateSemaphore(device, &semaCi, NULL, &semas[i]) );
    }
}

void onyx_CreateFences(VkDevice device, bool signaled, int count, VkFence* fences)
{
    VkFenceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0
    };

    for (int i = 0; i < count; ++i) 
    {
        V_ASSERT( vkCreateFence(device, &ci, NULL, &fences[i]) );
    }
}

void onyx_CmdSetViewportScissor(VkCommandBuffer cmdbuf, u32 x, u32 y, u32 w, u32 h)
{
    VkViewport vp = {
        .width = w,
        .height = h,
        .x = x, .y = y,
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };
    VkRect2D sz = {
        .extent = {w, h},
        .offset = {x, y}
    };

    vkCmdSetViewport(cmdbuf, 0, 1, &vp);
    vkCmdSetScissor(cmdbuf, 0, 1, &sz);
}

void onyx_CmdSetViewportScissorFull(VkCommandBuffer cmdbuf, unsigned width, unsigned height)
{
    VkViewport vp = {
        .width = width,
        .height = height,
        .x = 0, .y = 0,
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };
    VkRect2D sz = {
        .extent = {width, height},
        .offset = {0, 0}
    };

    vkCmdSetViewport(cmdbuf, 0, 1, &vp);
    vkCmdSetScissor(cmdbuf, 0, 1, &sz);
}

void onyx_CmdBeginRenderPass_ColorDepth(VkCommandBuffer cmdbuf, 
        const VkRenderPass renderPass, const VkFramebuffer framebuffer,
        unsigned width, unsigned height,
        float r, float g, float b, float a)
{
    VkClearValue clears[2] = {
        {r, g, b, a},
        {1.0, 0} //depth
    };

    VkRenderPassBeginInfo rpi = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = framebuffer,
        .renderArea = {0, 0, width, height},
        .clearValueCount = 2,
        .pClearValues = clears
    };

    vkCmdBeginRenderPass(cmdbuf, &rpi, VK_SUBPASS_CONTENTS_INLINE);
}

void onyx_CmdEndRenderPass(VkCommandBuffer cmdbuf)
{
    vkCmdEndRenderPass(cmdbuf);
}

void onyx_CmdClearColorImage(VkCommandBuffer cmdbuf, VkImage image, VkImageLayout layout,
        uint32_t base_mip_level, uint32_t mip_level_count,
        float r, float g, float b, float a)
{
    VkClearColorValue color = {
        .float32[0] = r,
        .float32[1] = g,
        .float32[2] = b,
        .float32[3] = a,
    };

    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel = base_mip_level,
        .layerCount = 1,
        .levelCount = mip_level_count
    };

    vkCmdClearColorImage(cmdbuf, image, layout, &color, 1, &range);
}

