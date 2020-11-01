#ifndef TANTO_R_RENDERPASS_H
#define TANTO_R_RENDERPASS_H

#include "v_def.h"

#define TANTO_R_MAX_ATTACHMENTS 8
#define TANTO_R_MAX_ATTACHMENT_REFERENCES 8
#define TANTO_R_MAX_SUBPASSES 8

typedef enum {
    TANTO_R_ATTACHMENT_COLOR_TYPE,
    TANTO_R_ATTACHMENT_DEPTH_TYPE,
} Tanto_R_AttachmentType;

typedef enum {
    TANTO_R_ATTACHMENT_REFERENCE_COLOR_TYPE,
    TANTO_R_ATTACHMENT_REFERENCE_DEPTH_STENCIL_TYPE,
    TANTO_R_ATTACHMENT_REFERENCE_RESOLVE_TYPE,
    TANTO_R_ATTACHMENT_REFERENCE_INPUT_TYPE,
} Tanto_R_AttachmentReferenceType;

typedef struct {
    Tanto_R_AttachmentType type;
    VkFormat               format;
    VkSampleCountFlags     sampleCount;
} Tanto_R_AttachmentDescription;

typedef struct {
    uint32_t                        attachmentNumber;
    Tanto_R_AttachmentReferenceType type;
} Tanto_R_AttachmentReference;

typedef Tanto_R_AttachmentReference Tanto_R_Subpass[TANTO_R_MAX_ATTACHMENT_REFERENCES];

typedef struct {
    uint32_t attachmentCount;
    Tanto_R_AttachmentDescription attachmensts[TANTO_R_MAX_ATTACHMENTS];
    uint32_t subpassCount;
    Tanto_R_Subpass               subpass[TANTO_R_MAX_SUBPASSES];
} Tanto_R_RenderPassInfo;

void tanto_r_CreateRenderPass(VkRenderPass* pRenderPass);

#endif /* end of include guard: TANTO_R_RENDERPASS_H */
