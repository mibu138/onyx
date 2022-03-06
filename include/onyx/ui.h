#ifndef ONYX_U_UI_H
#define ONYX_U_UI_H

#include <stdint.h>
#include <hell/input.h>
#include "geo.h"

typedef struct Onyx_U_Widget Onyx_U_Widget;
typedef Onyx_U_Widget Onyx_Widget;
typedef struct Onyx_UI Onyx_UI;

uint64_t       onyx_SizeOfUI(void);
Onyx_UI* onyx_AllocUI(void);
void           onyx_u_DebugReport(const Onyx_UI*);
Onyx_U_Widget* onyx_CreateSimpleBoxWidget(Onyx_UI* ui, const int16_t x, const int16_t y, 
                        const int16_t width, const int16_t height, Onyx_Widget* parent);
Onyx_U_Widget* onyx_CreateSliderWidget(Onyx_UI* ui, const int16_t x, const int16_t y, 
                        Onyx_Widget* parent);
Onyx_U_Widget* onyx_CreateTextWidget(Onyx_UI* ui, const int16_t x, const int16_t y, const char* text,
                        Onyx_Widget* parent);
void           onyx_UpdateWidgetText(Onyx_Widget* widget, const char* text);
void onyx_DestroyWidget(Onyx_UI* ui, Onyx_Widget* widget);

const VkSemaphore onyx_GetUISemaphore(Onyx_UI* ui, uint32_t frameIndex);
const VkFence     onyx_GetUIFence(Onyx_UI* ui, uint32_t frameIndex);

void onyx_CreateUI(Onyx_Memory* memory, Hell_EventQueue* queue, Hell_Window* window,
              const VkFormat imageFormat, const VkImageLayout inputLayout,
              const VkImageLayout finalLayout, const uint32_t imageViewCount,
              const VkImageView views[/*imageViewCount*/], Onyx_UI* ui);
void onyx_DestroyUI(Onyx_UI*, const uint32_t imgCount);

void onyx_RenderUI(Onyx_UI* ui, const uint32_t frameIndex, const uint32_t width,
              const uint32_t height, VkCommandBuffer cmdBuf);

void
onyx_RecreateSwapchainDependentUI(Onyx_UI*, const uint32_t width, const uint32_t height,
                                  const uint32_t    imageViewCount,
                                  const VkImageView views[/*imageViewCount*/]);

#endif /* end of include guard: ONYX_U_UI_H */
