#ifndef TANTO_V_COMMAND_H
#define TANTO_V_COMMAND_H

#include "v_def.h"
#include "v_video.h"

typedef struct Tanto_V_Command{
    VkCommandPool       pool;
    VkCommandBuffer     buffer;
    VkSemaphore         semaphore;
    VkFence             fence;
    uint32_t            queueFamily;
} Tanto_V_Command;

typedef struct {
    VkPipelineStageFlags srcStageFlags;
    VkPipelineStageFlags dstStageFlags;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
} Tanto_V_Barrier;

Tanto_V_Command tanto_v_CreateCommand(const Tanto_V_QueueType);

void            tanto_v_BeginCommandBuffer(VkCommandBuffer cmdBuf);
void            tanto_v_EndCommandBuffer(VkCommandBuffer cmdBuf);

void            tanto_v_SubmitAndWait(Tanto_V_Command* cmd, const uint32_t queueIndex);

void            tanto_v_DestroyCommand(Tanto_V_Command);

#endif /* end of include guard: TANTO_V_COMMAND_H */
