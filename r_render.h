#ifndef TANTO_R_INIT_H
#define TANTO_R_INIT_H

#include "v_def.h"
#include "v_memory.h"
#include <coal/coal.h>

#define TANTO_VERT_POS_FORMAT VK_FORMAT_R32G32B32_SFLOAT
#define TANTO_VERT_INDEX_TYPE VK_INDEX_TYPE_UINT32

typedef struct frame {
    VkCommandPool       commandPool;
    VkCommandBuffer     commandBuffer;
    VkSemaphore         semaphore;
    VkFence             fence;
    Tanto_V_Image       swapImage;
    uint32_t            index;
} Tanto_R_Frame;

VkFormat tanto_r_GetOffscreenColorFormat(void);
VkFormat tanto_r_GetDepthFormat(void);
VkFormat tanto_r_GetSwapFormat(void);

VkRenderPass tanto_r_GetSwapchainRenderPass(void);
VkRenderPass tanto_r_GetOffscreenRenderPass(void);
VkRenderPass tanto_r_GetMSAARenderPass(void);

void           tanto_r_Init(void);
void           tanto_r_WaitOnQueueSubmit(void);
void           tanto_r_WaitOnFrame(int8_t frameIndex);
bool           tanto_r_PresentFrame(void);
void           tanto_r_CleanUp(void);
void           tanto_r_RecreateSwapchain(void);
Tanto_R_Frame* tanto_r_GetFrame(const int8_t index);
const int8_t   tanto_r_RequestFrame(void); // returns available frame index. -1 on out of date

#endif /* end of include guard: R_INIT_H */
