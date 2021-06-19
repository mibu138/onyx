#ifndef OBDN_U_UI_H
#define OBDN_U_UI_H

#include <stdint.h>
#include <vulkan/vulkan_core.h>
#include <hell/input.h>
#include "geo.h"

typedef struct Obdn_U_Widget Obdn_U_Widget;
typedef Obdn_U_Widget Obdn_Widget;
typedef struct Obdn_UI Obdn_UI;

uint64_t       obdn_SizeOfUI(void);
Obdn_UI* obdn_AllocUI(void);
void           obdn_u_DebugReport(const Obdn_UI*);
Obdn_U_Widget* obdn_CreateSimpleBoxWidget(Obdn_UI* ui, const int16_t x, const int16_t y, 
                        const int16_t width, const int16_t height, Obdn_Widget* parent);
Obdn_U_Widget* obdn_CreateSliderWidget(Obdn_UI* ui, const int16_t x, const int16_t y, 
                        Obdn_Widget* parent);
Obdn_U_Widget* obdn_CreateTextWidget(Obdn_UI* ui, const int16_t x, const int16_t y, const char* text,
                        Obdn_Widget* parent);
void           obdn_UpdateWidgetText(Obdn_Widget* widget, const char* text);
void obdn_DestroyWidget(Obdn_UI* ui, Obdn_Widget* widget);

const VkSemaphore obdn_GetUISemaphore(Obdn_UI* ui, uint32_t frameIndex);
const VkFence     obdn_GetUIFence(Obdn_UI* ui, uint32_t frameIndex);

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
