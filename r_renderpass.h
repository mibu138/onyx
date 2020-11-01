#ifndef TANTO_R_RENDERPASS_H
#define TANTO_R_RENDERPASS_H

#include "v_def.h"

#define TANTO_R_MAX_ATTACHMENTS 8
#define TANTO_R_MAX_ATTACHMENT_REFERENCES 8
#define TANTO_R_MAX_SUBPASSES 8

typedef struct {
    uint32_t attachmentCount;
    uint32_t subpassCount;
    VkAttachmentDescription* pAttachments;
    VkSubpassDescription*    pSubpasses;
} Tanto_R_RenderPassInfo;

void tanto_r_CreateRenderPass(const Tanto_R_RenderPassInfo* info, VkRenderPass* pRenderPass);

#endif /* end of include guard: TANTO_R_RENDERPASS_H */
