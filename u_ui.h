#ifndef TANTO_U_UI_H
#define TANTO_U_UI_H

#include <stdint.h>
#include "i_input.h"
#include "r_geo.h"

typedef struct Tanto_U_Widget Tanto_U_Widget;

#define MAX_WIDGETS_PER_WIDGET 8
#define MAX_PRIMS_PER_WIDGET   8

typedef bool (*Tanto_U_ResponderFn)(const Tanto_I_Event* event, Tanto_U_Widget* widget);
typedef void (*Tanto_U_DrawFn)(const VkCommandBuffer cmdBuf, const Tanto_U_Widget* widget);

typedef struct {
    float sliderPos;
} Tanto_U_SliderData;

typedef union {
    Tanto_U_SliderData slider;
} Tanto_U_WidgetData;

struct Tanto_U_Widget {   
    Tanto_R_Primitive    primitives[MAX_PRIMS_PER_WIDGET];
    Tanto_U_Widget*      widgets[MAX_WIDGETS_PER_WIDGET];
    Tanto_U_ResponderFn  inputHandlerFn;
    Tanto_U_DrawFn       drawFn;
    Tanto_U_WidgetData   data;
    int16_t              x;
    int16_t              y;
    int16_t              width;
    int16_t              height;
    uint8_t              primCount;
    uint8_t              widgetCount;
    uint8_t              id;
};

void tanto_u_Init(void);
void tanto_u_DebugReport(void);
Tanto_U_Widget* tanto_u_CreateSimpleBox(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, Tanto_U_Widget* parent);
Tanto_U_Widget* tanto_u_CreateSlider(const int16_t x, const int16_t y, 
        Tanto_U_Widget* parent);
uint8_t tanto_u_GetWidgets(const Tanto_U_Widget** pToFirst);
void tanto_u_render(const VkCommandBuffer cmdBuf, const uint8_t frameIndex);

#endif /* end of include guard: TANTO_U_UI_H */