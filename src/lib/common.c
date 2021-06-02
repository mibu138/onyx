#include "common.h"
#include "v_swapchain.h"
#include <hell/common.h>
#include <hell/cmd.h>
#include <stdarg.h>
#include <stdio.h>
#include "v_video.h"
#include "u_ui.h"
#include "v_private.h"

#define MAX_PRINT_MSG 256
#define HEADER "OBSIDIAN: "

#define STR_100_MB "104857600"
#define STR_256_MB "268435456"

void obdn_Announce(const char* fmt, ...)
{
    va_list argptr;
    char    msg[MAX_PRINT_MSG];
    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);
    hell_Print("%s%s", HEADER, msg);
}

void obdn_CreateFramebuffer(const Obdn_Instance* instance, const unsigned attachmentCount, const VkImageView* attachments, 
        const unsigned width, const unsigned height, 
        const VkRenderPass renderpass,
        VkFramebuffer* framebuffer)
{
    VkFramebufferCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderpass,
        .attachmentCount = attachmentCount,
        .pAttachments = attachments,
        .width = width,
        .height = height,
        .layers = 1
    };
    vkCreateFramebuffer(instance->device, &ci, NULL, framebuffer);
}

void obdn_DestroyFramebuffer(const Obdn_Instance* instance, VkFramebuffer fb)
{
    vkDestroyFramebuffer(instance->device, fb, NULL);
}

