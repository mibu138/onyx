/*
r_render.c
 */
#ifndef R_INIT_H
#define R_INIT_H

#include "v_def.h"
#include "v_memory.h"
#include "m_math.h"

#define R_VERT_POS_FORMAT VK_FORMAT_R32G32B32_SFLOAT
#define R_VERT_INDEX_TYPE VK_INDEX_TYPE_UINT32

typedef struct {
    VkImage         handle;
    VkImageView     view;
    VkSampler       sampler;
} Image;

typedef struct {
    VkFramebuffer   handle;
    Image           colorAttachment;
    Image           depthAttachment;
    VkRenderPass*   pRenderPass;
} FrameBuffer;

typedef struct frame {
    VkCommandPool   commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore     semaphore;
    VkFence         fence;
    VkImage*        pImage;
    VkImageView     imageView;
    VkFramebuffer   frameBuffer;
    VkRenderPass*   renderPass;
    uint32_t        index;
} Frame;

extern VkPipeline pipelines[MAX_PIPELINES];
extern VkRenderPass swapchainRenderPass;
extern VkRenderPass offscreenRenderPass;
extern Frame frames[FRAME_COUNT];
extern uint32_t curFrameIndex;
extern FrameBuffer offscreenFrameBuffer;

void   r_Init(void);
void   r_WaitOnQueueSubmit(void);
Frame* r_RequestFrame(void);
void   r_PresentFrame(void);
void   r_CleanUp(void);

#endif /* end of include guard: R_INIT_H */
