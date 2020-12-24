#include "u_ui.h"
#include "tanto/r_geo.h"
#include "i_input.h"
#include <stdio.h>

#define MAX_WIDGETS 256 // might actually be a good design constraint

typedef Tanto_U_Widget Widget;

typedef struct {
    int16_t initX;
    int16_t initY;
    int16_t clickX;
    int16_t clickY;
    bool    active;
} DragData;

static uint32_t widgetCount;
static Widget   widgets[MAX_WIDGETS];
static Widget*  rootWidget;

static DragData dragData[MAX_WIDGETS];

static inline Widget* addWidget(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, 
        const Tantu_U_ResponderFn rfn, Widget* parent)
{
    widgets[widgetCount] = (Widget){
        .id = widgetCount,
        .x = 0, .y = 0,
        .width = width,
        .height = height,
        .inputHandlerFn = rfn
    };

    if (parent)
        parent->widgets[parent->widgetCount++] = &widgets[widgetCount];

    return &widgets[widgetCount++];
}

static inline bool clickTest(int16_t x, int16_t y, Widget* widget)
{
    x -= widget->x;
    y -= widget->y;
    bool xtest = (x > 0 && x < widget->width);
    bool ytest = (y > 0 && y < widget->height);
    return (xtest && ytest);
}

static inline void updateWidgetPos(const int16_t newx, const int16_t newy, Widget* widget)
{
    const int16_t deltaX = newx - widget->x;
    const int16_t deltaY = newy - widget->y;
    widget->x = newx;
    widget->y = newy;
    for (int i = 0; i < widget->primCount; i++) 
    {
        Tanto_R_Primitive* prim = &widget->primitives[i];
        Tanto_R_Attribute* pos = tanto_r_GetPrimAttribute(prim, 0);
        for (int i = 0; i < prim->vertexCount; i++) 
        {
            pos[i].i += deltaX;
            pos[i].j += deltaY;
        }
    }
    // TODO: must update children as well
}

static bool rfnSimpleBox(const Tanto_I_Event* event, Widget* widget)
{
    const uint8_t id = widget->id;

    switch (event->type)
    {
        case TANTO_I_MOUSEDOWN: 
        {
            const int16_t mx = event->data.mouseData.x;
            const int16_t my = event->data.mouseData.y;
            const bool r = clickTest(mx, my, widget);
            if (!r) return false;
            dragData[id].active = r;
            dragData[id].initX = widget->x;
            dragData[id].initY = widget->y;
            dragData[id].clickX = mx;
            dragData[id].clickY = my;
            return true;
        }
        case TANTO_I_MOUSEUP: dragData[id].active = false; break;
        case TANTO_I_MOTION: 
        {
            if (!dragData[id].active) return false;
            const int16_t mx = event->data.mouseData.x;
            const int16_t my = event->data.mouseData.y;
            const int16_t newx = dragData[id].initX + (mx - dragData[id].clickX);
            const int16_t newy = dragData[id].initY + (my - dragData[id].clickY);
            updateWidgetPos(newx, newy, widget);
            return true;
        }
        default: break;
    }
    return false;
}

static bool rfnPassThrough(const Tanto_I_Event* event, Widget* widget)
{
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = 0; i < widgetCount; i++) 
    {
        const bool r = widget->widgets[i]->inputHandlerFn(event, widgets->widgets[i]);
        if (r) return true;
    }
    return false;
}

static bool responder(const Tanto_I_Event* event)
{
    return widgets[0].inputHandlerFn(event, &widgets[0]);
}

void tanto_u_Init(void)
{
    widgetCount = 0;
    rootWidget = addWidget(0, 0, TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT, rfnPassThrough, NULL);

    tanto_i_Subscribe(responder);
    printf("Tanto UI initialized.\n");
}

Tanto_U_Widget* tanto_u_CreateSimpleBox(void)
{
    Widget* widget = addWidget(0, 0, 200, 100, rfnSimpleBox, rootWidget);

    widget->primCount = 1;
    widget->primitives[0] = tanto_r_CreateQuadNDC(widget->x, widget->y, widget->width, widget->height, NULL);

    return widget;
}

void tanto_u_DebugReport(void)
{
    printf("========== Tanto_U_Report ==========\n");
    printf("Number of widgets: %d\n", widgetCount);
    for (int i = 0; i < widgetCount; i++) 
    {
        Widget* widget = &widgets[i];
        printf("Widget %d:\n", i);
        printf("\tID: %d\n", widget->id);
        printf("\tPos: %d, %d\n", widget->x, widget->y);
        printf("\tDim: %d, %d\n", widget->width, widget->height);
    }
}

uint8_t tanto_u_GetWidgets(const Tanto_U_Widget** pToFirst)
{
    *pToFirst = widgets;
    return widgetCount;
}
