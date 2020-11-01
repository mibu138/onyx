#include "r_renderpass.h"

#include "v_video.h"

void tanto_r_CreateRenderPass(const Tanto_R_RenderPassInfo *info, VkRenderPass *pRenderPass)
{
    // only allow one subpass for now
    assert(info->subpassCount == 1);

    const VkSubpassDependency dependency0 = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    const VkSubpassDependency dependency1 = {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkSubpassDependency dependencies[] = {
        dependency0, dependency1
    };

    VkRenderPassCreateInfo ci = {
        .subpassCount = 1,
        .pSubpasses = info->pSubpasses,
        .attachmentCount = info->attachmentCount,
        .pAttachments = info->pAttachments,
        .dependencyCount = 2,
        .pDependencies = dependencies,
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    };

    V_ASSERT( vkCreateRenderPass(device, &ci, NULL, pRenderPass) );

}
