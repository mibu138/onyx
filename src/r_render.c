#include "s_scene.h"

// TODO: we should implement a way to specify the offscreen format
static const VkFormat offscreenColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
static const VkFormat depthFormat          = VK_FORMAT_D32_SFLOAT;

void obdn_r_DrawScene(const VkCommandBuffer cmdBuf, const Obdn_S_Scene* scene)
{
    for (int i = 0; i < scene->primCount; i++) 
    {
        obdn_r_DrawPrim(cmdBuf, &scene->prims[i].rprim);
    }
}

VkFormat obdn_r_GetOffscreenColorFormat(void) { return offscreenColorFormat; }
VkFormat obdn_r_GetDepthFormat(void)          { return depthFormat; }
