#include "u_ui.h"
#include "tanto/r_geo.h"
#include "tanto/r_render.h"
#include "tanto/r_renderpass.h"
#include "i_input.h"
#include <stdio.h>

#define MAX_WIDGETS 256 // might actually be a good design constraint

typedef Tanto_U_Widget Widget;

typedef struct {
    int16_t prevX;
    int16_t prevY;
    bool    active;
} DragData;

static uint32_t widgetCount;
static Widget   widgets[MAX_WIDGETS];
static Widget*  rootWidget;

static DragData dragData[MAX_WIDGETS];

static VkRenderPass renderPassUI;

static VkPipeline pipelineSlider;
static VkPipeline pipelineSimpleBox;

static inline void initRenderPass(void)
{
    const VkAttachmentDescription attachmentColor = {
        .flags = 0,
        .format = tanto_r_GetSwapFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    const VkAttachmentDescription attachments[] = {
        attachmentColor
    };

    VkAttachmentReference colorReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass = {
        .flags                   = 0,
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount    = 0,
        .pInputAttachments       = NULL,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &colorReference,
        .pResolveAttachments     = NULL,
        .pDepthStencilAttachment = NULL,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments    = NULL,
    };

    Tanto_R_RenderPassInfo rpi = {
        .attachmentCount = 1,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    tanto_r_CreateRenderPass(&rpi, &renderPassUI);
}

static inline void initPipelines(void)
{
}

static inline Widget* addWidget(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, 
        const Tantu_U_ResponderFn rfn, Widget* parent)
{
    widgets[widgetCount] = (Widget){
        .id = widgetCount,
        .x = x, .y = y,
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

static inline void updateWidgetPos(const int16_t deltaX, const int16_t deltaY, Widget* widget)
{
    widget->x += deltaX;
    widget->y += deltaY;
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

    // update children
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = widgetCount - 1; i >= 0; i--) 
    {
        updateWidgetPos(deltaX, deltaY, widget->widgets[i]);
    }
}

static bool propogateEventToChildren(const Tanto_I_Event* event, Widget* widget)
{
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = widgetCount - 1; i >= 0 ; i--) 
    {
        const bool r = widget->widgets[i]->inputHandlerFn(event, widget->widgets[i]);
        if (r) return true;
    }
    return false;
}

static bool rfnSimpleBox(const Tanto_I_Event* event, Widget* widget)
{
    if (propogateEventToChildren(event, widget)) return true;
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
            dragData[id].prevX = mx;
            dragData[id].prevY = my;
            return true;
        }
        case TANTO_I_MOUSEUP: dragData[id].active = false; break;
        case TANTO_I_MOTION: 
        {
            if (!dragData[id].active) return false;
            const int16_t mx = event->data.mouseData.x;
            const int16_t my = event->data.mouseData.y;
            const int16_t deltaX = mx - dragData[id].prevX;
            const int16_t deltaY = my - dragData[id].prevY;
            dragData[id].prevX = mx;
            dragData[id].prevY = my;
            updateWidgetPos(deltaX, deltaY, widget);
            return true;
        }
        default: break;
    }
    return false;
}

static bool rfnPassThrough(const Tanto_I_Event* event, Widget* widget)
{
    return propogateEventToChildren(event, widget);
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

Tanto_U_Widget* tanto_u_CreateSimpleBox(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, Widget* parent)
{
    if (!parent) parent = rootWidget;
    Widget* widget = addWidget(x, y, width, height, rfnSimpleBox, parent);

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
