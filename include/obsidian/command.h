#ifndef OBDN_V_COMMAND_H
#define OBDN_V_COMMAND_H

#include "def.h"
#include "video.h"

typedef struct Obdn_V_Command{
    VkCommandPool        pool;
    VkCommandBuffer      buffer;
    VkSemaphore          semaphore;
    VkFence              fence;
    uint32_t             queueFamily;
    const Obdn_Instance* instance;
} Obdn_Command;

typedef struct {
    VkPipelineStageFlags srcStageFlags;
    VkPipelineStageFlags dstStageFlags;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
} Obdn_Barrier;

typedef struct Obdn_CommandPool {
    VkCommandPool    pool;
    VkCommandBuffer* cmdbufs;
    uint32_t         queueFamily;
    uint32_t         cmdbuf_count;
} Obdn_CommandPool;

Obdn_Command obdn_CreateCommand(const Obdn_Instance* instance, const Obdn_V_QueueType);

Obdn_CommandPool obdn_CreateCommandPool(VkDevice device,
    uint32_t queueFamilyIndex,
    VkCommandPoolCreateFlags poolflags,
    uint32_t bufcount);
void obdn_DestroyCommandPool(VkDevice device, Obdn_CommandPool* pool);
void obdn_BeginCommandBuffer(VkCommandBuffer cmdBuf);
void obdn_BeginCommandBufferOneTimeSubmit(VkCommandBuffer cmdBuf);
void obdn_EndCommandBuffer(VkCommandBuffer cmdBuf);

void obdn_SubmitAndWait(Obdn_Command* cmd, const uint32_t queueIndex);

void obdn_DestroyCommand(Obdn_Command);

void obdn_WaitForFence(VkDevice device, VkFence* fence);
void obdn_ResetCommand(Obdn_Command* cmd);
void obdn_WaitForFenceNoReset(VkDevice device, VkFence* fence);
void obdn_v_MemoryBarrier(
    VkCommandBuffer      commandBuffer,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkDependencyFlags    dependencyFlags,
    VkAccessFlags        srcAccessMask,
    VkAccessFlags        dstAccessMask);

void obdn_CreateFence(VkDevice, VkFence* fence);
void obdn_CreateFences(VkDevice device, bool signaled, int count, VkFence* fences);
void obdn_CreateSemaphore(VkDevice, VkSemaphore* semaphore);
void obdn_CreateSemaphores(VkDevice device, u32 count, VkSemaphore* semas);

void obdn_SubmitCommand(Obdn_Command* cmd, VkSemaphore semaphore, VkFence fence);

void obdn_CmdSetViewportScissor(VkCommandBuffer cmdbuf, u32 x, u32 y, u32 w, u32 h);
void obdn_CmdSetViewportScissorFull(VkCommandBuffer cmdbuf, unsigned width, unsigned height);

void obdn_CmdBeginRenderPass_ColorDepth(VkCommandBuffer cmdbuf, 
        const VkRenderPass renderPass, const VkFramebuffer framebuffer,
        unsigned width, unsigned height,
        float r, float g, float b, float a);

void obdn_CmdEndRenderPass(VkCommandBuffer cmdbuf);

void obdn_DestroyFence(VkDevice device, VkFence fence);
void obdn_DestroySemaphore(VkDevice device, VkSemaphore semaphore);

void obdn_CmdClearColorImage(VkCommandBuffer cmdbuf, VkImage image, VkImageLayout layout,
        uint32_t base_mip_level, uint32_t mip_level_count,
        float r, float g, float b, float a);

#endif /* end of include guard: OBDN_V_COMMAND_H */
