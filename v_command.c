#include "v_command.h"
#include "tanto/v_video.h"
#include <vulkan/vulkan_core.h>

void tanto_v_SubmitAndWait(Tanto_V_Command* cmd, const uint32_t queueIndex)
{
    tanto_v_SubmitToQueueWait(&cmd->buffer, cmd->queueFamily, queueIndex);
}

Tanto_V_Command tanto_v_CreateCommand(const Tanto_V_QueueType queueFamilyType)
{
    Tanto_V_Command cmd = {
        .queueFamily = tanto_v_GetQueueFamilyIndex(queueFamilyType),
    };

    const VkCommandPoolCreateInfo cmdPoolCi = {
        .queueFamilyIndex = cmd.queueFamily,
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };

    V_ASSERT( vkCreateCommandPool(device, &cmdPoolCi, NULL, &cmd.pool) );

    const VkCommandBufferAllocateInfo allocInfo = {
        .commandBufferCount = 1,
        .commandPool = cmd.pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
    };

    V_ASSERT( vkAllocateCommandBuffers(device, &allocInfo, &cmd.buffer) );

    const VkSemaphoreCreateInfo semaCi = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    V_ASSERT( vkCreateSemaphore(device, &semaCi, NULL, &cmd.semaphore) );

    const VkFenceCreateInfo fenceCi = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    V_ASSERT( vkCreateFence(device, &fenceCi, NULL, &cmd.fence) );

    return cmd;
}

void tanto_v_BeginCommandBuffer(VkCommandBuffer cmdBuf)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    V_ASSERT( vkBeginCommandBuffer(cmdBuf, &beginInfo) );
}

void tanto_v_EndCommandBuffer(VkCommandBuffer cmdBuf)
{
    vkEndCommandBuffer(cmdBuf);
}


void tanto_v_WaitForFence(VkFence* fence)
{
    vkWaitForFences(device, 1, fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, fence);
}

void tanto_v_DestroyCommand(Tanto_V_Command cmd)
{
    vkDestroyCommandPool(device, cmd.pool, NULL);
    vkDestroyFence(device, cmd.fence, NULL);
    vkDestroySemaphore(device, cmd.semaphore, NULL);
}

void tanto_v_ResetCommand(Tanto_V_Command* cmd)
{
    vkResetCommandPool(device, cmd->pool, 0);
}
