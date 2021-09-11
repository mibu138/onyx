#define COAL_SIMPLE_TYPE_NAMES
#include "ui.h"
#include "common.h"
#include "dtags.h"
#include "geo.h"
#include "pipeline.h"
#include "render.h"
#include "renderpass.h"
#include "text.h"
#include "command.h"
#include "image.h"
#include "memory.h"
#include "private.h"
#include "swapchain.h"
#include "video.h"
#include <hell/common.h>
#include <hell/debug.h>
#include <hell/input.h>
#include <hell/len.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WIDGETS 256 // might actually be a good design constraint

#define SPVDIR "./shaders/spv"
#define MAX_IMAGE_COUNT 8

typedef Obdn_U_Widget Widget;
typedef Obdn_Image  Image;

typedef struct {
    int i0;
    int i1;
} PushConstantVert;

typedef struct {
    int   i0;
    int   i1;
    float f0;
    float f1;
} PushConstantFrag;

enum { PIPELINE_BOX, PIPELINE_SLIDER, PIPELINE_TEXTURE, PIPELINE_COUNT };

_Static_assert(PIPELINE_COUNT < OBDN_MAX_PIPELINES, "");

typedef struct Obdn_UI {
    Widget*               rootWidget;
    VkRenderPass          renderPass;
    VkPipelineLayout      pipelineLayout;
    VkPipeline            pipelines[PIPELINE_COUNT];
    VkDescriptorSetLayout descriptorSetLayout;
    Obdn_R_Description    descriptions[OBDN_FRAME_COUNT];
    VkFramebuffer         framebuffers[OBDN_FRAME_COUNT];
    Image                 images[MAX_IMAGE_COUNT];
    Obdn_Memory*          memory;
    VkDevice              device;
} Obdn_UI;

#define MAX_WIDGETS_PER_WIDGET 8
#define MAX_PRIMS_PER_WIDGET 8
#define OBDN_U_MAX_TEXT_SIZE 31

typedef bool (*Obdn_U_ResponderFn)(const Hell_Event* event,
                                   Obdn_U_Widget*    widget);
typedef void (*Obdn_U_DrawFn)(const VkCommandBuffer cmdBuf,
                              const Obdn_U_Widget*  widget);
typedef void (*Obdn_U_DestructFn)(Obdn_U_Widget* widget);

typedef struct {
    float sliderPos;
} Obdn_U_SliderData;

typedef struct {
    uint8_t imageIndex;
    char    text[OBDN_U_MAX_TEXT_SIZE];
} Obdn_U_TextData;

typedef union {
    Obdn_U_SliderData slider;
    Obdn_U_TextData   text;
} Obdn_U_WidgetData;

typedef struct {
    int16_t prevX;
    int16_t prevY;
    bool    active;
} Obdn_U_DragData;

struct Obdn_U_Widget {
    Obdn_Geometry   primitives[MAX_PRIMS_PER_WIDGET];
    Obdn_U_Widget*     widgets[MAX_WIDGETS_PER_WIDGET]; // unordered
    Obdn_U_Widget*     parent;
    Obdn_U_ResponderFn inputHandlerFn;
    Obdn_U_DrawFn      drawFn;
    Obdn_U_DestructFn  destructFn;
    int16_t            x;
    int16_t            y;
    int16_t            width;
    int16_t            height;
    Obdn_U_DragData    dragData;
    uint8_t            primCount;
    uint8_t            widgetCount;
    Obdn_U_WidgetData  data;
    Obdn_UI*           ui;
};

_Static_assert(
    MAX_IMAGE_COUNT <= 256,
    "Max image count cannot be greater than 256 to fit in imageCount");

#define DPRINT(fmt, ...) hell_DebugPrint(OBDN_DEBUG_TAG_UI, fmt, ##__VA_ARGS__);

static Widget*
allocWidget(void)
{
    return hell_Malloc(sizeof(Widget));
}

static void
freeWidget(Widget* w)
{
    memset(w, 0, sizeof(Widget));
    free(w);
    w = NULL;
}

static void
initRenderPass(Obdn_UI* ui, const VkFormat format,
               const VkImageLayout initialLayout,
               const VkImageLayout finalLayout)
{
    obdn_CreateRenderPass_Color(ui->memory->instance->device, initialLayout,
                                finalLayout, VK_ATTACHMENT_LOAD_OP_LOAD, format,
                                &ui->renderPass);
}

static void
initDescriptionsAndPipelineLayouts(Obdn_UI* ui)
{
    Obdn_DescriptorBinding binding = {
             .descriptorCount = MAX_IMAGE_COUNT,
             .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
             .bindingFlags    = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

    const Obdn_DescriptorSetInfo dsInfo = {
        .bindingCount = 1,
        .bindings     = &binding};

    obdn_CreateDescriptorSetLayouts(ui->memory->instance->device, 1, &dsInfo,
                                    &ui->descriptorSetLayout);

    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
    {
        obdn_CreateDescriptorSets(ui->memory->instance->device, 1, &dsInfo,
                                  &ui->descriptorSetLayout,
                                  &ui->descriptions[i]);
    }

    VkPushConstantRange pcRanges[] = {
        {.offset     = 0,
         .size       = sizeof(PushConstantFrag),
         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
        {.offset     = sizeof(PushConstantFrag),
         .size       = sizeof(PushConstantVert),
         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT}};

    const Obdn_PipelineLayoutInfo pipelayoutInfos[] = {
        {.descriptorSetCount   = 1,
         .descriptorSetLayouts = &ui->descriptorSetLayout,
         .pushConstantCount    = LEN(pcRanges),
         .pushConstantsRanges  = pcRanges}};

    obdn_CreatePipelineLayouts(ui->memory->instance->device,
                               LEN(pipelayoutInfos), pipelayoutInfos,
                               &ui->pipelineLayout);
}

static void
updateTexture(const Obdn_UI* ui, uint8_t imageIndex)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
    {
        VkDescriptorImageInfo textureInfo = {
            .imageLayout = ui->images[imageIndex].layout,
            .imageView   = ui->images[imageIndex].view,
            .sampler     = ui->images[imageIndex].sampler};

        VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext           = NULL,
            .dstSet          = ui->descriptions[i].descriptorSets[0],
            .dstBinding      = 0,
            .dstArrayElement = imageIndex,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo      = &textureInfo};

        vkUpdateDescriptorSets(ui->memory->instance->device, 1, &write, 0,
                               NULL);
    }
}

static void
initPipelines(Obdn_UI* ui, uint32_t width, uint32_t height)
{
    Obdn_GeoAttributeSize        attrSizes[2] = {12, 12};
    Obdn_GraphicsPipelineInfo pipeInfos[]  = {
        {// simple box
         .renderPass        = ui->renderPass,
         .layout            = ui->pipelineLayout,
         .sampleCount       = VK_SAMPLE_COUNT_1_BIT,
         .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
         .frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE,
         .blendMode         = OBDN_R_BLEND_MODE_OVER,
         .vertexDescription = obdn_GetVertexDescription(2, attrSizes),
         .viewportDim       = {width, height},
         .vertShader        = "obsidian/ui.vert.spv",
         .fragShader        = "obsidian/ui-box.frag.spv"},
        {// slider
         .renderPass        = ui->renderPass,
         .layout            = ui->pipelineLayout,
         .sampleCount       = VK_SAMPLE_COUNT_1_BIT,
         .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
         .frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE,
         .blendMode         = OBDN_R_BLEND_MODE_OVER,
         .viewportDim       = {width, height},
         .vertexDescription = obdn_GetVertexDescription(2, attrSizes),
         .vertShader        = "obsidian/ui.vert.spv",
         .fragShader        = "obsidian/ui-slider.frag.spv"},
        {// texture
         .renderPass        = ui->renderPass,
         .layout            = ui->pipelineLayout,
         .sampleCount       = VK_SAMPLE_COUNT_1_BIT,
         .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
         .frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE,
         .blendMode         = OBDN_R_BLEND_MODE_OVER,
         .viewportDim       = {width, height},
         .vertexDescription = obdn_GetVertexDescription(2, attrSizes),
         .vertShader        = "obsidian/ui.vert.spv",
         .fragShader        = "obsidian/ui-texture.frag.spv"}};

    obdn_CreateGraphicsPipelines(ui->memory->instance->device, LEN(pipeInfos),
                                 pipeInfos, ui->pipelines);
}

static Widget*
addWidget(Obdn_UI* ui, const int16_t x, const int16_t y, const int16_t width,
          const int16_t height, const Obdn_U_ResponderFn inputHanderFn,
          const Obdn_U_DrawFn drawFn, const Obdn_U_DestructFn destructFn,
          Widget* parent)
{
    Widget* widget = allocWidget();
    *widget        = (Widget){.x              = x,
                       .y              = y,
                       .width          = width,
                       .height         = height,
                       .inputHandlerFn = inputHanderFn,
                       .drawFn         = drawFn,
                       .destructFn     = destructFn,
                       .ui             = ui};

    if (!parent && ui->rootWidget) // if no parent given and we have root
                                   // widget, make widget its child
        parent = ui->rootWidget;

    if (parent)
    {
        parent->widgets[parent->widgetCount++] = widget;
        widget->parent                         = parent;
        assert(parent->widgetCount < MAX_WIDGETS_PER_WIDGET);
    }

    return widget;
}

static bool
clickTest(int16_t x, int16_t y, Widget* widget)
{
    x -= widget->x;
    y -= widget->y;
    bool xtest = (x > 0 && x < widget->width);
    bool ytest = (y > 0 && y < widget->height);
    return (xtest && ytest);
}

static void
updateWidgetPos(const int16_t dx, const int16_t dy, Widget* widget)
{
    widget->x += dx;
    widget->y += dy;
    for (int i = 0; i < widget->primCount; i++)
    {
        Obdn_Geometry* prim = &widget->primitives[i];
        Vec3*             pos  = obdn_GetGeoAttribute(prim, 0);
        for (int i = 0; i < prim->vertexCount; i++)
        {
            pos[i].x += dx;
            pos[i].y += dy;
        }
    }

    // update children
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = widgetCount - 1; i >= 0; i--)
    {
        updateWidgetPos(dx, dy, widget->widgets[i]);
    }
}

static bool
propogateEventToChildren(const Hell_Event* event, Widget* widget)
{
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = widgetCount - 1; i >= 0; i--)
    {
        if (widget->widgets[i]->inputHandlerFn)
        {
            const bool r =
                widget->widgets[i]->inputHandlerFn(event, widget->widgets[i]);
            if (r)
                return true;
        }
    }
    return false;
}

static void
dfnSimpleBox(const VkCommandBuffer cmdBuf, const Widget* widget)
{
    assert(widget->primCount == 1);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      widget->ui->pipelines[PIPELINE_BOX]);

    const Obdn_Geometry* prim = &widget->primitives[0];

    PushConstantFrag pc = {.i0 = widget->width, .i1 = widget->height};

    vkCmdPushConstants(cmdBuf, widget->ui->pipelineLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

    obdn_DrawGeo(cmdBuf, prim);
}

static bool
rfnSimpleBox(const Hell_Event* event, Widget* widget)
{
    if (propogateEventToChildren(event, widget))
        return true;

    switch (event->type)
    {
    case HELL_EVENT_TYPE_MOUSEDOWN: {
        const int16_t mx = hell_GetMouseX(event);
        const int16_t my = hell_GetMouseY(event);
        const bool    r  = clickTest(mx, my, widget);
        if (!r)
            return false;
        widget->dragData.active = r;
        widget->dragData.prevX  = mx;
        widget->dragData.prevY  = my;
        return true;
    }
    case HELL_EVENT_TYPE_MOUSEUP:
        widget->dragData.active = false;
        break;
    case HELL_EVENT_TYPE_MOTION: {
        if (!widget->dragData.active)
            return false;
        const int16_t mx       = hell_GetMouseX(event);
        const int16_t my       = hell_GetMouseY(event);
        const int16_t dx       = mx - widget->dragData.prevX;
        const int16_t dy       = my - widget->dragData.prevY;
        widget->dragData.prevX = mx;
        widget->dragData.prevY = my;
        updateWidgetPos(dx, dy, widget);
        return true;
    }
    default:
        break;
    }
    return false;
}

static void
dfnSlider(const VkCommandBuffer cmdBuf, const Widget* widget)
{
    assert(widget->primCount == 1);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      widget->ui->pipelines[PIPELINE_SLIDER]);

    const Obdn_Geometry* prim = &widget->primitives[0];

    PushConstantFrag pc = {.i0 = widget->width,
                           .i1 = widget->height,
                           .f0 = widget->data.slider.sliderPos};

    vkCmdPushConstants(cmdBuf, widget->ui->pipelineLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

    obdn_DrawGeo(cmdBuf, prim);
}

static bool
rfnSlider(const Hell_Event* event, Widget* widget)
{
    switch (event->type)
    {
    case HELL_EVENT_TYPE_MOUSEDOWN: {
        const int16_t mx = hell_GetMouseX(event);
        const int16_t my = hell_GetMouseY(event);
        const bool    r  = clickTest(mx, my, widget);
        if (!r)
            return false;
        widget->dragData.active       = r;
        widget->data.slider.sliderPos = (float)(mx - widget->x) / widget->width;
        return true;
    }
    case HELL_EVENT_TYPE_MOUSEUP:
        widget->dragData.active = false;
        break;
    case HELL_EVENT_TYPE_MOTION: {
        if (!widget->dragData.active)
            return false;
        const int16_t mx              = hell_GetMouseX(event);
        widget->data.slider.sliderPos = (float)(mx - widget->x) / widget->width;
        return true;
    }
    default:
        break;
    }
    return false;
}

static void
dfnText(const VkCommandBuffer cmdBuf, const Widget* widget)
{
    assert(widget->primCount == 1);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      widget->ui->pipelines[PIPELINE_TEXTURE]);

    const Obdn_Geometry* prim = &widget->primitives[0];

    PushConstantFrag pc = {.i0 = widget->data.text.imageIndex};

    vkCmdPushConstants(cmdBuf, widget->ui->pipelineLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

    obdn_DrawGeo(cmdBuf, prim);
}

static void
destructText(Widget* text)
{
    obdn_FreeImage(&text->ui->images[text->data.text.imageIndex]);
}

static bool
rfnPassThrough(const Hell_Event* event, Widget* widget)
{
    return propogateEventToChildren(event, widget);
}

static bool
responder(const Hell_Event* event, void* uiPtr)
{
    Obdn_UI* ui = (Obdn_UI*)uiPtr;
    return ui->rootWidget->inputHandlerFn(event, ui->rootWidget);
}

static void
initFrameBuffers(Obdn_UI* ui, uint32_t width, uint32_t height,
                 const uint32_t    imageViewCount,
                 const VkImageView views[imageViewCount])
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
    {
        const VkImageView attachments[] = {views[i]};

        const VkFramebufferCreateInfo fbi = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext           = NULL,
            .flags           = 0,
            .renderPass      = ui->renderPass,
            .attachmentCount = 1,
            .pAttachments    = attachments,
            .width           = width,
            .height          = height,
            .layers          = 1,
        };

        V_ASSERT(vkCreateFramebuffer(ui->memory->instance->device, &fbi, NULL,
                                     &ui->framebuffers[i]));
    }
}

static void
destroyPipelines(Obdn_UI* ui)
{
    for (int i = 0; i < PIPELINE_COUNT; i++)
    {
        vkDestroyPipeline(ui->memory->instance->device, ui->pipelines[i], NULL);
    }
}

static void
destroyFramebuffers(Obdn_UI* ui, const uint32_t imgCount)
{
    for (int i = 0; i < imgCount; i++)
    {
        vkDestroyFramebuffer(ui->memory->instance->device, ui->framebuffers[i],
                             NULL);
    }
}

void
obdn_RecreateSwapchainDependentUI(Obdn_UI* ui, const uint32_t width,
                                  const uint32_t    height,
                                  const uint32_t    imageViewCount,
                                  const VkImageView views[imageViewCount])
{
    destroyPipelines(ui);
    destroyFramebuffers(ui, imageViewCount);
    initPipelines(ui, width, height);
    initFrameBuffers(ui, width, height, imageViewCount, views);
}

// does not modify parent
static void
destroyWidget(Widget* widget)
{
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = 0; i < widgetCount; i++)
    {
        destroyWidget(widget->widgets[i]); // destroy children. careful, this
                                           // can modify widgetCount
    }
    for (int i = 0; i < widget->primCount; i++)
    {
        obdn_FreeGeo(&widget->primitives[i]);
    }
    if (widget->destructFn)
        widget->destructFn(widget);
    freeWidget(widget);
}

static int
requestImageIndex(const Obdn_UI* ui)
{
    for (int i = 0; i < MAX_IMAGE_COUNT; i++)
    {
        if (ui->images[i].size == 0) // available
            return i;
    }
    return -1;
}

void
obdn_CreateUI(Obdn_Memory* memory, Hell_EventQueue* queue, Hell_Window* window,
              const VkFormat imageFormat, const VkImageLayout inputLayout,
              const VkImageLayout finalLayout, const uint32_t imageViewCount,
              const VkImageView views[imageViewCount], Obdn_UI* ui)
{
    assert(ui);
    memset(ui, 0, sizeof(Obdn_UI));
    ui->memory    = memory;
    ui->device    = memory->instance->device;
    VkExtent2D ex = {hell_GetWindowWidth(window), hell_GetWindowHeight(window)};
    initRenderPass(ui, imageFormat, inputLayout, finalLayout);
    initDescriptionsAndPipelineLayouts(ui);
    initPipelines(ui, ex.width, ex.height);
    initFrameBuffers(ui, ex.width, ex.height, imageViewCount, views);

    ui->rootWidget = addWidget(ui, 0, 0, ex.width, ex.height, rfnPassThrough,
                               NULL, NULL, NULL);

    hell_Subscribe(queue, HELL_EVENT_MASK_MOUSE_BIT | HELL_EVENT_MASK_KEY_BIT,
                   hell_GetWindowID(window), responder, ui);
    obdn_Announce("UI initialized.\n");
}

Obdn_U_Widget*
obdn_CreateSimpleBoxWidget(Obdn_UI* ui, const int16_t x, const int16_t y,
                           const int16_t width, const int16_t height,
                           Widget* parent)
{
    Widget* widget = addWidget(ui, x, y, width, height, rfnSimpleBox,
                               dfnSimpleBox, NULL, parent);

    widget->primCount     = 1;
    widget->primitives[0] = obdn_CreateQuadNDC(ui->memory, widget->x, widget->y,
                                               widget->width, widget->height);

    return widget;
}

Obdn_U_Widget*
obdn_CreateSliderWidget(Obdn_UI* ui, const int16_t x, const int16_t y,
                        Widget* parent)
{
    Widget* widget =
        addWidget(ui, x, y, 300, 40, rfnSlider, dfnSlider, NULL, parent);
    DPRINT("Slider X %d Y %d\n", x, y);

    widget->primCount     = 1;
    widget->primitives[0] = obdn_CreateQuadNDC(ui->memory, widget->x, widget->y,
                                               widget->width, widget->height);

    widget->data.slider.sliderPos = 0.5;

    return widget;
}

Obdn_U_Widget*
obdn_CreateTextWidget(Obdn_UI* ui, const int16_t x, const int16_t y,
                      const char* text, Widget* parent)
{
    const int charCount = strlen(text);
    const int width     = charCount * 25;
    Widget*   widget =
        addWidget(ui, x, y, width, 100, NULL, dfnText, destructText, parent);
    DPRINT("UI: Text widget X %d Y %d\n", x, y);

    widget->primCount     = 1;
    widget->primitives[0] = obdn_CreateQuadNDC(ui->memory, widget->x, widget->y,
                                               widget->width, widget->height);
    strncpy(widget->data.text.text, text, OBDN_U_MAX_TEXT_SIZE);

    const int imgId = requestImageIndex(ui);
    assert(imgId != -1 && "No image available");
    ui->images[imgId] = obdn_t_CreateTextImage(ui->memory, width, 100, 0, 50,
                                               36, widget->data.text.text);
    widget->data.text.imageIndex = imgId;
    updateTexture(ui, imgId);

    return widget;
}

void
obdn_u_UpdateText(const char* text, Widget* widget)
{
    vkDeviceWaitIdle(widget->ui->device);
    obdn_t_UpdateTextImage(widget->ui->memory, 0, 50, text,
                           &widget->ui->images[widget->data.text.imageIndex]);
    updateTexture(widget->ui, widget->data.text.imageIndex);
}

static void
widgetReport(Widget* widget)
{
    hell_Print("Widget %p:\n", widget);
    hell_Print("\tPos: %d, %d\n", widget->x, widget->y);
    hell_Print("\tDim: %d, %d\n", widget->width, widget->height);
    hell_Print("\tChildCount: %d\n", widget->widgetCount);
    for (int i = 0; i < widget->widgetCount; i++)
    {
        widgetReport(widget->widgets[i]);
    }
}

void
obdn_u_DebugReport(const Obdn_UI* ui)
{
    hell_Print("========== Obdn_U_Report ==========\n");
    widgetReport(ui->rootWidget);
}

static void
drawWidget(const VkCommandBuffer cmdBuf, Widget* widget)
{
    for (int i = 0; i < widget->widgetCount; i++)
    {
        drawWidget(cmdBuf, widget->widgets[i]);
    }
    if (widget->drawFn)
        widget->drawFn(cmdBuf, widget);
}

void
obdn_RenderUI(Obdn_UI* ui, const uint32_t frameIndex, const uint32_t width,
              const uint32_t height, VkCommandBuffer cmdBuf)
{
    VkClearValue clear = {0};

    const VkRenderPassBeginInfo rpassInfoUi = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 1,
        .pClearValues    = &clear,
        .renderArea      = {{0, 0}, {width, height}},
        .renderPass      = ui->renderPass,
        .framebuffer     = ui->framebuffers[frameIndex],
    };

    vkCmdBeginRenderPass(cmdBuf, &rpassInfoUi, VK_SUBPASS_CONTENTS_INLINE);

    PushConstantVert pc = {.i0 = width, .i1 = height};

    vkCmdPushConstants(cmdBuf, ui->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       sizeof(PushConstantFrag), sizeof(PushConstantVert), &pc);

    vkCmdBindDescriptorSets(
        cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ui->pipelineLayout, 0,
        ui->descriptions[frameIndex].descriptorSetCount,
        ui->descriptions[frameIndex].descriptorSets, 0, NULL);

    drawWidget(cmdBuf, ui->rootWidget);

    vkCmdEndRenderPass(cmdBuf);
}

// does modify parent
void
obdn_DestroyWidget(Obdn_UI* ui, Widget* widget)
{
    int     wi     = -1;
    Widget* parent = widget->parent;
    if (!parent)
        parent = ui->rootWidget;
    for (int i = 0; i < parent->widgetCount; i++)
    {
        if (parent->widgets[i] == widget)
            wi = i;
    }
    if (wi != -1)
    {
        destroyWidget(widget);
        parent->widgets[wi] = parent->widgets[--parent->widgetCount];
    }
    else
    {
        assert(0 && "Widget is not a child of its own parent...");
    }
}

void
obdn_DestroyUI(Obdn_UI* ui, const uint32_t imgCount)
{
    destroyFramebuffers(ui, imgCount);
    vkDestroyPipelineLayout(ui->device, ui->pipelineLayout, NULL);
    destroyPipelines(ui);
    vkDestroyRenderPass(ui->device, ui->renderPass, NULL);
    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
    {
        obdn_DestroyDescription(ui->device, &ui->descriptions[i]);
    }
    for (int i = 0; i < MAX_IMAGE_COUNT; i++)
    {
        if (ui->images[i].size != 0)
            obdn_FreeImage(&ui->images[i]);
    }
    vkDestroyDescriptorSetLayout(ui->device, ui->descriptorSetLayout, NULL);
    destroyWidget(ui->rootWidget);
    memset(ui, 0, sizeof(Obdn_UI));
    obdn_Announce("UI cleaned up.\n");
}

uint64_t
obdn_SizeOfUI(void)
{
    return sizeof(Obdn_UI);
}

Obdn_UI* obdn_AllocUI(void)
{
    return hell_Malloc(sizeof(Obdn_UI));
}
