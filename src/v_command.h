#ifndef OBDN_V_COMMAND_H
#define OBDN_V_COMMAND_H

#include "v_def.h"
#include "v_video.h"

typedef struct Obdn_V_Command{
    VkCommandPool       pool;
    VkCommandBuffer     buffer;
    VkSemaphore         semaphore;
    VkFence             fence;
    uint32_t            queueFamily;
} Obdn_V_Command;

typedef struct {
    VkPipelineStageFlags srcStageFlags;
    VkPipelineStageFlags dstStageFlags;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
} Obdn_V_Barrier;

Obdn_V_Command obdn_v_CreateCommand(const Obdn_V_QueueType);

void obdn_v_BeginCommandBuffer(VkCommandBuffer cmdBuf);
void obdn_v_EndCommandBuffer(VkCommandBuffer cmdBuf);

void obdn_v_SubmitAndWait(Obdn_V_Command* cmd, const uint32_t queueIndex);

void obdn_v_DestroyCommand(Obdn_V_Command);

void obdn_v_WaitForFence(VkFence* fence);
void obdn_v_ResetCommand(Obdn_V_Command* cmd);
void obdn_v_WaitForFenceNoReset(VkFence* fence);
void obdn_v_MemoryBarrier(
    VkCommandBuffer      commandBuffer,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkDependencyFlags    dependencyFlags,
    VkAccessFlags        srcAccessMask,
    VkAccessFlags        dstAccessMask);

void obdn_CreateFence(VkFence* fence);
void obdn_CreateSemaphore(VkSemaphore* semaphore);

void obdn_SubmitCommand(Obdn_V_Command* cmd, VkSemaphore semaphore, VkFence fence);

void obdn_CmdSetViewportScissorFull(VkCommandBuffer cmdbuf, unsigned width, unsigned height);

void obdn_CmdBeginRenderPass_ColorDepth(VkCommandBuffer cmdbuf, 
        const VkRenderPass renderPass, const VkFramebuffer framebuffer,
        unsigned width, unsigned height,
        float r, float g, float b, float a);

void obdn_CmdEndRenderPass(VkCommandBuffer cmdbuf);

#endif /* end of include guard: OBDN_V_COMMAND_H */
