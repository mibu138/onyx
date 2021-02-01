#ifndef OBDN_U_UI_H
#define OBDN_U_UI_H

#include <stdint.h>
#include "i_input.h"
#include "r_geo.h"

typedef struct Obdn_U_Widget Obdn_U_Widget;

#define MAX_WIDGETS_PER_WIDGET 8
#define MAX_PRIMS_PER_WIDGET   8

typedef bool (*Obdn_U_ResponderFn)(const Obdn_I_Event* event, Obdn_U_Widget* widget);
typedef void (*Obdn_U_DrawFn)(const VkCommandBuffer cmdBuf, const Obdn_U_Widget* widget);

typedef struct {
    float sliderPos;
} Obdn_U_SliderData;

typedef union {
    Obdn_U_SliderData slider;
} Obdn_U_WidgetData;

struct Obdn_U_Widget {   
    Obdn_R_Primitive    primitives[MAX_PRIMS_PER_WIDGET];
    Obdn_U_Widget*      widgets[MAX_WIDGETS_PER_WIDGET];
    Obdn_U_ResponderFn  inputHandlerFn;
    Obdn_U_DrawFn       drawFn;
    Obdn_U_WidgetData   data;
    int16_t              x;
    int16_t              y;
    int16_t              width;
    int16_t              height;
    uint8_t              primCount;
    uint8_t              widgetCount;
    uint8_t              id;
};

void               obdn_u_Init(const VkImageLayout inputLayout, const VkImageLayout finalLayout);
void               obdn_u_DebugReport(void);
Obdn_U_Widget*    obdn_u_CreateSimpleBox(const int16_t x, const int16_t y, 
                        const int16_t width, const int16_t height, Obdn_U_Widget* parent);
Obdn_U_Widget*    obdn_u_CreateSlider(const int16_t x, const int16_t y, 
                        Obdn_U_Widget* parent);
uint8_t            obdn_u_GetWidgets(const Obdn_U_Widget** pToFirst);
VkSemaphore        obdn_u_Render(const VkSemaphore waitSemephore);
void               obdn_u_CleanUp(void);

#endif /* end of include guard: OBDN_U_UI_H */
