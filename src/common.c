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

void obdn_Init(void)
{
    Obdn_V_Config cfg = {0};
    cfg.memorySizes.hostGraphicsBufferMemorySize   = hell_c_GetVar("hstGraphicsBufferMemorySize", STR_100_MB, HELL_C_VAR_ARCHIVE_BIT)->value;
    cfg.memorySizes.deviceGraphicsBufferMemorySize = hell_c_GetVar("devGraphicsBufferMemorySize", STR_100_MB, HELL_C_VAR_ARCHIVE_BIT)->value;
    cfg.memorySizes.deviceGraphicsImageMemorySize  =  hell_c_GetVar("devGraphicsImageSize", STR_256_MB, HELL_C_VAR_ARCHIVE_BIT)->value;
#ifdef NDEBUG
    cfg.validationEnabled = false;
#else
    cfg.validationEnabled = true;
#endif
    obdn_v_Init(&cfg, 0, NULL);
}

void obdn_InitUI(void)
{
    obdn_u_Init(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

unsigned obdn_CreateSwapchain(const struct Hell_Window* window)
{
    obdn_v_InitSwapchain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, window);
    return 0;
}

void obdn_CreateFramebuffer(const unsigned attachmentCount, const VkImageView* attachments, 
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

void obdn_DestroyFramebuffer(VkFramebuffer fb)
{
    vkDestroyFramebuffer(device, fb, NULL);
}

const Obdn_Image* obdn_GetSwapchainImage(const unsigned int swapchainId, const int8_t imageIndex)
{
    return obdn_v_GetFrame(imageIndex);
}

unsigned obdn_GetSurfaceWidth(void)
{
    return obdn_v_GetSwapExtent().width;
}

unsigned obdn_GetSurfaceHeight(void)
{
    return obdn_v_GetSwapExtent().height;
}
