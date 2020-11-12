#ifndef TANTO_R_INIT_H
#define TANTO_R_INIT_H

#include "v_def.h"
#include "v_memory.h"
#include "m_math.h"

#define TANTO_VERT_POS_FORMAT VK_FORMAT_R32G32B32_SFLOAT
#define TANTO_VERT_INDEX_TYPE VK_INDEX_TYPE_UINT32

typedef struct {
    VkFramebuffer   handle;
    Tanto_V_Image   colorAttachment;
    Tanto_V_Image   depthAttachment;
    Tanto_V_Image   resolveAttachment;
    VkRenderPass    renderPass;
} Tanto_R_FrameBuffer;

typedef struct frame {
    VkCommandPool       commandPool;
    VkCommandBuffer     commandBuffer;
    VkSemaphore         semaphore;
    VkFence             fence;
    Tanto_V_Image       swapImage;
    uint32_t            index;
} Tanto_R_Frame;

typedef Tanto_V_Image Tanto_R_Image; //for now 

extern VkRenderPass swapchainRenderPass;
extern VkRenderPass offscreenRenderPass;
extern VkRenderPass msaaRenderPass;
extern Tanto_R_Frame frames[TANTO_FRAME_COUNT];
extern uint32_t curFrameIndex;
extern const VkFormat offscreenColorFormat;
extern const VkFormat depthFormat;

void           tanto_r_Init(void);
void           tanto_r_WaitOnQueueSubmit(void);
bool           tanto_r_PresentFrame(void);
void           tanto_r_CleanUp(void);
Tanto_R_Frame* tanto_r_RequestFrame(void);

#endif /* end of include guard: R_INIT_H */
