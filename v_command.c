#include "v_command.h"
#include "v_video.h"

Tanto_V_CommandPool tanto_v_RequestOneTimeUseCommand(const uint32_t queueIndex)
{
    Tanto_V_CommandPool pool;

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueIndex,
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
