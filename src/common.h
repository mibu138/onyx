#ifndef OBSIDIAN_COMMON_H
#define OBSIDIAN_COMMON_H

#include "v_vulkan.h"
#include "v_image.h"
#include "v_command.h"

struct Hell_Window;

typedef Obdn_V_Image   Obdn_Image;
typedef Obdn_V_Command Obdn_Command;

void obdn_Announce(const char *fmt, ...);
void obdn_Init(void);

// if null is passed it will be an offscreen swapchain.
// returns the swapchain id. currently not used for anything.
unsigned obdn_CreateSwapchain(const struct Hell_Window *window);

void obdn_CreateFramebuffer(const unsigned attachmentCount,
                            const VkImageView *attachments,
                            const unsigned width, const unsigned height,
                            const VkRenderPass renderpass,
                            VkFramebuffer *framebuffer);

void obdn_DestroyFramebuffer(VkFramebuffer fb);

// swapchainId is not used currently. just put 0.
const Obdn_Image* obdn_GetSwapchainImage(const unsigned swapchainId, const int8_t imageIndex);

unsigned obdn_GetSurfaceWidth(void);
unsigned obdn_GetSurfaceHeight(void);

#endif /* end of include guard: OBSIDIAN_COMMON_H */
