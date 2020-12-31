#ifndef TANTO_V_COMMAND_H
#define TANTO_V_COMMAND_H

#include "v_def.h"
#include "v_video.h"

typedef struct {
    VkCommandPool   handle;
    VkCommandBuffer buffer;
    uint32_t        queueFamily;
} Tanto_V_CommandPool;

typedef struct {
    VkCommandPool       commandPool;
    VkCommandBuffer     commandBuffer;
    VkSemaphore         semaphore;
    VkFence             fence;
} Tanto_V_Command;

Tanto_V_CommandPool tanto_v_RequestOneTimeUseCommand(void);
Tanto_V_CommandPool tanto_v_RequestCommandPool(Tanto_V_QueueType queueFamilyType);

void tanto_v_SubmitOneTimeCommandAndWait(Tanto_V_CommandPool* pool, const uint32_t queueIndex);
void tanto_v_SubmitAndWait(Tanto_V_CommandPool* pool, const uint32_t queueIndex);

Tanto_V_Command tanto_v_CreateCommand(void);
void tanto_v_DestroyCommand(Tanto_V_Command);

#endif /* end of include guard: TANTO_V_COMMAND_H */
