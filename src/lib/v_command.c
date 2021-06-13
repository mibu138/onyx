#include "v_command.h"
#include "v_video.h"
#include "v_private.h"

void obdn_SubmitAndWait(Obdn_V_Command* cmd, const uint32_t queueIndex)
{
    obdn_SubmitToQueueWait(cmd->instance, &cmd->buffer, cmd->queueFamily, queueIndex);
}

Obdn_V_Command obdn_CreateCommand(const Obdn_Instance* instance, const Obdn_V_QueueType queueFamilyType)
{
    Obdn_V_Command cmd = {
        .queueFamily = obdn_GetQueueFamilyIndex(instance, queueFamilyType),
        .instance = instance
    };

    const VkCommandPoolCreateInfo cmdPoolCi = {
        .queueFamilyIndex = cmd.queueFamily,
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

void obdn_BeginCommandBuffer(VkCommandBuffer cmdBuf)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    V_ASSERT( vkBeginCommandBuffer(cmdBuf, &beginInfo) );
}

void obdn_BeginCommandBufferOneTimeSubmit(VkCommandBuffer cmdBuf)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    V_ASSERT( vkBeginCommandBuffer(cmdBuf, &beginInfo) );
}

void obdn_EndCommandBuffer(VkCommandBuffer cmdBuf)
{
    vkEndCommandBuffer(cmdBuf);
}


void obdn_WaitForFence(VkDevice device, VkFence* fence)
{
    vkWaitForFences(device, 1, fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, fence);
}

void obdn_DestroyFence(VkDevice device, VkFence fence)
{
    vkDestroyFence(device, fence, NULL);
}

void obdn_DestroySemaphore(VkDevice device, VkSemaphore semaphore)
{
    vkDestroySemaphore(device, semaphore, NULL);
}

void obdn_WaitForFenceNoReset(VkDevice device, VkFence* fence)
{
    vkWaitForFences(device, 1, fence, VK_TRUE, UINT64_MAX);
}

void obdn_DestroyCommand(Obdn_V_Command cmd)
{
    vkDestroyCommandPool(cmd.instance->device, cmd.pool, NULL);
    vkDestroyFence(cmd.instance->device, cmd.fence, NULL);
    vkDestroySemaphore(cmd.instance->device, cmd.semaphore, NULL);
}

void obdn_ResetCommand(Obdn_V_Command* cmd)
{
    vkResetCommandPool(cmd->instance->device, cmd->pool, 0);
}

void obdn_v_MemoryBarrier(
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

void obdn_CreateFence(VkDevice device, VkFence* fence)
{
    VkFenceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    V_ASSERT( vkCreateFence(device, &ci, NULL, fence) );
}

void obdn_CreateSemaphore(VkDevice device, VkSemaphore* semaphore)
{
    const VkSemaphoreCreateInfo semaCi = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    V_ASSERT( vkCreateSemaphore(device, &semaCi, NULL, semaphore) );
}

void obdn_CmdSetViewportScissorFull(VkCommandBuffer cmdbuf, unsigned width, unsigned height)
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

void obdn_CmdBeginRenderPass_ColorDepth(VkCommandBuffer cmdbuf, 
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

void obdn_CmdEndRenderPass(VkCommandBuffer cmdbuf)
{
    vkCmdEndRenderPass(cmdbuf);
}

