#ifndef ONYX_V_COMMAND_H
#define ONYX_V_COMMAND_H

#include "def.h"
#include "video.h"

typedef struct Onyx_V_Command{
    VkCommandPool        pool;
    VkCommandBuffer      buffer;
    VkSemaphore          semaphore;
    VkFence              fence;
    uint32_t             queueFamily;
    const Onyx_Instance* instance;
} Onyx_Command;

typedef struct {
    VkPipelineStageFlags srcStageFlags;
    VkPipelineStageFlags dstStageFlags;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
} Onyx_Barrier;

typedef struct Onyx_CommandPool {
    VkCommandPool    pool;
    VkCommandBuffer* cmdbufs;
    uint32_t         queueFamily;
    uint32_t         cmdbuf_count;
} Onyx_CommandPool;

Onyx_Command onyx_CreateCommand(const Onyx_Instance* instance, const Onyx_V_QueueType);

Onyx_CommandPool onyx_CreateCommandPool(VkDevice device,
    uint32_t queueFamilyIndex,
    VkCommandPoolCreateFlags poolflags,
    uint32_t bufcount);
void onyx_DestroyCommandPool(VkDevice device, Onyx_CommandPool* pool);
void onyx_BeginCommandBuffer(VkCommandBuffer cmdBuf);
void onyx_BeginCommandBufferOneTimeSubmit(VkCommandBuffer cmdBuf);
void onyx_EndCommandBuffer(VkCommandBuffer cmdBuf);

void onyx_SubmitAndWait(Onyx_Command* cmd, const uint32_t queueIndex);

void onyx_DestroyCommand(Onyx_Command);

void onyx_WaitForFence(VkDevice device, VkFence* fence);
void onyx_WaitForFence_TimeOut(VkDevice device, VkFence* fence, uint64_t timeout_ns);
void onyx_ResetCommand(Onyx_Command* cmd);
void onyx_WaitForFenceNoReset(VkDevice device, VkFence* fence);
void onyx_v_MemoryBarrier(
    VkCommandBuffer      commandBuffer,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkDependencyFlags    dependencyFlags,
    VkAccessFlags        srcAccessMask,
    VkAccessFlags        dstAccessMask);

void onyx_CreateFence(VkDevice, VkFence* fence);
void onyx_CreateFences(VkDevice device, bool signaled, int count, VkFence* fences);
void onyx_CreateSemaphore(VkDevice, VkSemaphore* semaphore);
void onyx_CreateSemaphores(VkDevice device, u32 count, VkSemaphore* semas);

void onyx_SubmitCommand(Onyx_Command* cmd, VkSemaphore semaphore, VkFence fence);

void onyx_CmdSetViewportScissor(VkCommandBuffer cmdbuf, u32 x, u32 y, u32 w, u32 h);
void onyx_CmdSetViewportScissorFull(VkCommandBuffer cmdbuf, unsigned width, unsigned height);

void onyx_CmdBeginRenderPass_ColorDepth(VkCommandBuffer cmdbuf, 
        const VkRenderPass renderPass, const VkFramebuffer framebuffer,
        unsigned width, unsigned height,
        float r, float g, float b, float a);

void onyx_CmdEndRenderPass(VkCommandBuffer cmdbuf);

void onyx_DestroyFence(VkDevice device, VkFence fence);
void onyx_DestroySemaphore(VkDevice device, VkSemaphore semaphore);

void onyx_CmdClearColorImage(VkCommandBuffer cmdbuf, VkImage image, VkImageLayout layout,
        uint32_t base_mip_level, uint32_t mip_level_count,
        float r, float g, float b, float a);

#endif /* end of include guard: ONYX_V_COMMAND_H */
