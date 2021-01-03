#ifndef TANTO_R_INIT_H
#define TANTO_R_INIT_H

#include "v_def.h"
#include "v_memory.h"
#include <coal/coal.h>
#include "v_command.h"
#include "s_scene.h"

#define TANTO_VERT_POS_FORMAT VK_FORMAT_R32G32B32_SFLOAT
#define TANTO_VERT_INDEX_TYPE VK_INDEX_TYPE_UINT32

typedef struct {
    Tanto_V_Command     command;
    Tanto_V_Image       swapImage;
    uint32_t            index;
} Tanto_R_Frame;

typedef void (*Tanto_R_SwapchainRecreationFn)(void);

VkFormat tanto_r_GetOffscreenColorFormat(void);
VkFormat tanto_r_GetDepthFormat(void);
VkFormat tanto_r_GetSwapFormat(void);

void           tanto_r_Init(void);
void           tanto_r_WaitOnQueueSubmit(void);
void           tanto_r_DrawScene(const VkCommandBuffer cmdBuf, const Tanto_S_Scene* scene);
void           tanto_r_WaitOnFrame(int8_t frameIndex);
bool           tanto_r_PresentFrame(void);
void           tanto_r_CleanUp(void);
void           tanto_r_RegisterSwapchainRecreationFn(Tanto_R_SwapchainRecreationFn fn);
void           tanto_r_RecreateSwapchain(void);
Tanto_R_Frame* tanto_r_GetFrame(const int8_t index);
const uint32_t tanto_r_GetCurrentFrameIndex(void);
const uint32_t tanto_r_RequestFrame(void); // returns available frame index.
void           tanto_r_SubmitFrame(void);
void           tanto_r_SubmitUI(const Tanto_V_Command cmd);

#endif /* end of include guard: R_INIT_H */
