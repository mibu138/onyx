#ifndef TANTO_U_UI_H
#define TANTO_U_UI_H

#include <stdint.h>
#include "i_input.h"
#include "r_geo.h"

typedef struct Tanto_U_Widget Tanto_U_Widget;

#define MAX_WIDGETS_PER_WIDGET 8
#define MAX_PRIMS_PER_WIDGET   8

typedef bool (*Tantu_U_ResponderFn)(const Tanto_I_Event* event, Tanto_U_Widget* widget);

struct Tanto_U_Widget {   
    Tanto_R_Primitive    primitives[MAX_PRIMS_PER_WIDGET];
    Tanto_U_Widget*      widgets[MAX_WIDGETS_PER_WIDGET];
    Tantu_U_ResponderFn  inputHandlerFn;
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
uint8_t tanto_u_GetWidgets(const Tanto_U_Widget** pToFirst);

#endif /* end of include guard: TANTO_U_UI_H */
