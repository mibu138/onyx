#ifndef OBDN_R_RENDERPASS_H
#define OBDN_R_RENDERPASS_H

#include "v_def.h"

#define OBDN_R_MAX_ATTACHMENTS 8
#define OBDN_R_MAX_ATTACHMENT_REFERENCES 8
#define OBDN_R_MAX_SUBPASSES 8

typedef struct {
    const uint32_t attachmentCount;
    const uint32_t subpassCount;
    const VkAttachmentDescription* pAttachments;
    const VkSubpassDescription*    pSubpasses;
} Obdn_R_RenderPassInfo;

void obdn_r_CreateRenderPass(const Obdn_R_RenderPassInfo* info, VkRenderPass* pRenderPass);
void obdn_r_CreateRenderPass_Color(const VkAttachmentLoadOp loadOp, 
        const VkImageLayout initialLayout, const VkImageLayout finalLayout,
        const VkFormat colorFormat,
        VkRenderPass* pRenderPass);
void obdn_r_CreateRenderPass_ColorDepth(
        const VkImageLayout colorInitialLayout, const VkImageLayout colorFinalLayout,
        const VkImageLayout depthInitialLayout, const VkImageLayout depthFinalLayout,
        const VkAttachmentLoadOp  colorLoadOp, const VkAttachmentStoreOp colorStoreOp,
        const VkAttachmentLoadOp  depthLoadOp, const VkAttachmentStoreOp depthStoreOp,
        const VkFormat colorFormat,
        const VkFormat depthFormat,
        VkRenderPass* pRenderPass);

void obdn_r_CreateRenderPass_ColorDepthMSAA(const VkSampleCountFlags sampleCount, const VkAttachmentLoadOp loadOp, 
        const VkImageLayout initialLayout, const VkImageLayout finalLayout,
        const VkFormat colorFormat,
        const VkFormat depthFormat,
        VkRenderPass* pRenderPass);

#endif /* end of include guard: OBDN_R_RENDERPASS_H */
