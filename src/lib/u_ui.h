#ifndef OBDN_U_UI_H
#define OBDN_U_UI_H

#include <stdint.h>
#include <vulkan/vulkan_core.h>
#include <hell/input.h>
#include "r_geo.h"

typedef struct Obdn_U_Widget Obdn_U_Widget;
typedef struct Obdn_UI Obdn_UI;

uint64_t       obdn_SizeOfUI(void);
void           obdn_u_DebugReport(const Obdn_UI*);
Obdn_U_Widget* obdn_CreateSimpleBoxWidget(Obdn_UI* ui, const int16_t x, const int16_t y, 
                        const int16_t width, const int16_t height, Obdn_U_Widget* parent);
Obdn_U_Widget* obdn_CreateSliderWidget(Obdn_UI* ui, const int16_t x, const int16_t y, 
                        Obdn_U_Widget* parent);
Obdn_U_Widget* obdn_CreateTextWidget(Obdn_UI* ui, const int16_t x, const int16_t y, const char* text,
                        Obdn_U_Widget* parent);
void           obdn_u_UpdateText(const char* text, Obdn_U_Widget* widget);
VkSemaphore    obdn_u_Render(const VkSemaphore waitSemephore);
void           obdn_u_CleanUp(void);
void           obdn_u_DestroyWidget(Obdn_U_Widget* widget);

const VkSemaphore obdn_u_GetSemaphore(Obdn_UI* ui, uint32_t frameIndex);
const VkFence     obdn_u_GetFence(Obdn_UI* ui, uint32_t frameIndex);

void obdn_CreateUI(Obdn_Memory* memory, Hell_EventQueue* queue, Hell_Window* window,
              const VkFormat imageFormat, const VkImageLayout inputLayout,
              const VkImageLayout finalLayout, const uint32_t imageViewCount,
              const VkImageView views[imageViewCount], Obdn_UI* ui);
void obdn_DestroyUI(Obdn_UI*, const uint32_t imgCount);
VkSemaphore obdn_RenderUI(Obdn_UI*, const uint32_t frameIndex, const uint32_t width, const uint32_t height, const VkSemaphore waitSemephore);

void
obdn_RecreateSwapchainDependentUI(Obdn_UI*, const uint32_t width, const uint32_t height,
                                  const uint32_t    imageViewCount,
                                  const VkImageView views[imageViewCount]);

#endif /* end of include guard: OBDN_U_UI_H */
