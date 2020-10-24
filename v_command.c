#include "v_command.h"
#include "v_video.h"
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

void tanto_v_SubmitOneTimeCommandAndWait(Tanto_V_CommandPool* pool, const uint32_t queueIndex)
{
    vkEndCommandBuffer(pool->buffer);

    tanto_v_SubmitToQueueWait(&pool->buffer, pool->queueFamily, queueIndex);

    vkDestroyCommandPool(device, pool->handle, NULL);
}
