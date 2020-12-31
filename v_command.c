#include "v_command.h"
#include <vulkan/vulkan_core.h>

Tanto_V_CommandPool tanto_v_RequestOneTimeUseCommand()
{
    Tanto_V_CommandPool pool;

    pool.queueFamily = TANTO_V_QUEUE_GRAPHICS_TYPE;

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = pool.queueFamily,
    };


    vkCreateCommandPool(device, &poolInfo, NULL, &pool.handle);

    VkCommandBufferAllocateInfo cmdBufInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandBufferCount = 1,
        .commandPool = pool.handle,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };

    vkAllocateCommandBuffers(device, &cmdBufInfo, &pool.buffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(pool.buffer, &beginInfo);

    return pool;
}

Tanto_V_CommandPool tanto_v_RequestCommandPool(Tanto_V_QueueType queueFamilyType)
{
    Tanto_V_CommandPool pool;

    pool.queueFamily = tanto_v_GetQueueFamilyIndex(queueFamilyType);

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = pool.queueFamily,
    };

    vkCreateCommandPool(device, &poolInfo, NULL, &pool.handle);

    VkCommandBufferAllocateInfo cmdBufInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandBufferCount = 1,
        .commandPool = pool.handle,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };

    vkAllocateCommandBuffers(device, &cmdBufInfo, &pool.buffer);

    return pool;
}

void tanto_v_SubmitAndWait(Tanto_V_CommandPool* pool, const uint32_t queueIndex)
{
    tanto_v_SubmitToQueueWait(&pool->buffer, pool->queueFamily, queueIndex);
}

void tanto_v_SubmitOneTimeCommandAndWait(Tanto_V_CommandPool* pool, const uint32_t queueIndex)
{
    vkEndCommandBuffer(pool->buffer);

    tanto_v_SubmitToQueueWait(&pool->buffer, pool->queueFamily, queueIndex);

    vkDestroyCommandPool(device, pool->handle, NULL);
}

Tanto_V_Command tanto_v_CreateCommand(void)
{
    Tanto_V_Command cmd;

    const VkCommandPoolCreateInfo cmdPoolCi = {
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    };

    V_ASSERT( vkCreateCommandPool(device, &cmdPoolCi, NULL, &cmd.commandPool) );

    const VkCommandBufferAllocateInfo allocInfo = {
        .commandBufferCount = 1,
        .commandPool = cmd.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
    };

    V_ASSERT( vkAllocateCommandBuffers(device, &allocInfo, &cmd.commandBuffer) );
    // spec states that the last parm is an array of commandBuffers... hoping a pointer
    // to a single one works just as well

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

void tanto_v_DestroyCommand(Tanto_V_Command cmd)
{
    vkDestroyCommandPool(device, cmd.commandPool, NULL);
    vkDestroyFence(device, cmd.fence, NULL);
    vkDestroySemaphore(device, cmd.semaphore, NULL);
}
