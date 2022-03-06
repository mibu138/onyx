#include "common.h"
#include "swapchain.h"
#include <hell/common.h>
#include <hell/cmd.h>
#include <stdarg.h>
#include <stdio.h>
#include "video.h"
#include "ui.h"
#include "private.h"

#define MAX_PRINT_MSG 256
#define HEADER "ONYX: "

#define STR_100_MB "104857600"
#define STR_256_MB "268435456"

void onyx_Announce(const char* fmt, ...)
{
    va_list argptr;
    char    msg[MAX_PRINT_MSG];
    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);
    hell_Print("%s%s", HEADER, msg);
}

void onyx_CreateFramebuffer(const VkDevice device, const unsigned attachmentCount, 
    const VkImageView* attachments, 
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
    vkCreateFramebuffer(device, &ci, NULL, framebuffer);
}

void onyx_DestroyFramebuffer(const VkDevice device, VkFramebuffer fb)
{
    vkDestroyFramebuffer(device, fb, NULL);
}

