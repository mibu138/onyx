#include "u_ui.h"
#include "obsidian/t_text.h"
#include "obsidian/v_image.h"
#include "obsidian/v_memory.h"
#include "r_geo.h"
#include "r_render.h"
#include "r_renderpass.h"
#include "r_pipeline.h"
#include "t_def.h"
#include "v_command.h"
#include "v_swapchain.h"
#include "v_video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hell/input.h>
#include "v_private.h"

#define MAX_WIDGETS 256 // might actually be a good design constraint

#define SPVDIR "./shaders/spv"
#define MAX_IMAGE_COUNT 8

typedef Obdn_U_Widget Widget;
typedef Obdn_V_Image  Image;

typedef struct {
    int   i0;
    int   i1;
} PushConstantVert;

typedef struct {
    int   i0;
    int   i1;
    float f0;
    float f1;
} PushConstantFrag;

enum {
    PIPELINE_BOX,
    PIPELINE_SLIDER,
    PIPELINE_TEXTURE,
    PIPELINE_COUNT
};
_Static_assert(PIPELINE_COUNT < OBDN_MAX_PIPELINES, "");

static Widget*  rootWidget;

static VkRenderPass     renderPass;

static VkPipelineLayout pipelineLayout;
static VkPipeline       pipelines[PIPELINE_COUNT];

static VkDescriptorSetLayout descriptorSetLayout;
static Obdn_R_Description    descriptions[OBDN_FRAME_COUNT];

static VkFramebuffer    framebuffers[OBDN_FRAME_COUNT];

_Static_assert(MAX_IMAGE_COUNT <= 256, "Max image count cannot be greater than 256 to fit in imageCount");

static Image   images[MAX_IMAGE_COUNT];

static Obdn_V_Command  renderCommands[OBDN_FRAME_COUNT];

static Widget* allocWidget(void)
{
    return malloc(sizeof(Widget));
}

static void freeWidget(Widget* w)
{
    memset(w, 0, sizeof(Widget));
    free(w);
    w = NULL;
}

static void initRenderCommands(void)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        renderCommands[i] = obdn_v_CreateCommand(OBDN_V_QUEUE_GRAPHICS_TYPE);
        printf("UI COMMAND BUF: %p\n", renderCommands[i].buffer);
    }
}

static void initRenderPass(const VkImageLayout initialLayout, const VkImageLayout finalLayout)
{
    obdn_r_CreateRenderPass_Color(initialLayout, finalLayout, VK_ATTACHMENT_LOAD_OP_LOAD, obdn_v_GetSwapFormat(), &renderPass);
}

static void initDescriptionsAndPipelineLayouts(void)
{
    const Obdn_R_DescriptorSetInfo dsInfo = {
        .bindingCount = 1,
        .bindings = {{
            .descriptorCount = MAX_IMAGE_COUNT,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
        }}
    };

    obdn_r_CreateDescriptorSetLayouts(1, &dsInfo, &descriptorSetLayout);

    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
    {
        obdn_r_CreateDescriptorSets(1, &dsInfo, &descriptorSetLayout, &descriptions[i]);
    }

    VkPushConstantRange pcRanges[] = {{
        .offset     = 0,
        .size       = sizeof(PushConstantFrag),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    },{
        .offset     = sizeof(PushConstantFrag),
        .size       = sizeof(PushConstantVert),
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    }};

    const Obdn_R_PipelineLayoutInfo pipelayoutInfos[] = {{
        .descriptorSetCount = 1,
        .descriptorSetLayouts = &descriptorSetLayout,
        .pushConstantCount   = OBDN_ARRAY_SIZE(pcRanges),
        .pushConstantsRanges = pcRanges
    }};

    obdn_r_CreatePipelineLayouts(OBDN_ARRAY_SIZE(pipelayoutInfos), 
            pipelayoutInfos, &pipelineLayout);
}

static void updateTexture(uint8_t imageIndex)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++)
    {
        VkDescriptorImageInfo textureInfo = {
            .imageLayout = images[imageIndex].layout,
            .imageView   = images[imageIndex].view,
            .sampler     = images[imageIndex].sampler
        };

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = descriptions[i].descriptorSets[0],
            .dstBinding = 0,
            .dstArrayElement = imageIndex,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &textureInfo
        };

        vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
    }
}

static void initPipelines(uint32_t width, uint32_t height)
{
    Obdn_R_AttributeSize attrSizes[2] = {12, 12};
    Obdn_R_GraphicsPipelineInfo pipeInfos[] = {{
        // simple box
        .renderPass        = renderPass,
        .layout            = pipelineLayout,
        .sampleCount       = VK_SAMPLE_COUNT_1_BIT,
        .frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .blendMode         = OBDN_R_BLEND_MODE_OVER,
        .vertexDescription = obdn_r_GetVertexDescription(2, attrSizes),
        .viewportDim       = {width, height},
        .vertShader        = "ui-vert.spv",
        .fragShader        = "ui-box-frag.spv"
    },{ 
        // slider
        .renderPass        = renderPass,
        .layout            = pipelineLayout,
        .sampleCount       = VK_SAMPLE_COUNT_1_BIT,
        .frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .blendMode         = OBDN_R_BLEND_MODE_OVER,
        .viewportDim       = {width, height},
        .vertexDescription = obdn_r_GetVertexDescription(2, attrSizes),
        .vertShader        = "ui-vert.spv",
        .fragShader        = "ui-slider-frag.spv"
    },{ 
        // texture
        .renderPass        = renderPass,
        .layout            = pipelineLayout,
        .sampleCount       = VK_SAMPLE_COUNT_1_BIT,
        .frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .blendMode         = OBDN_R_BLEND_MODE_OVER,
        .viewportDim       = {width, height},
        .vertexDescription = obdn_r_GetVertexDescription(2, attrSizes),
        .vertShader        = "ui-vert.spv",
        .fragShader        = "ui-texture-frag.spv"
    }};

    obdn_r_CreateGraphicsPipelines(OBDN_ARRAY_SIZE(pipeInfos), pipeInfos, pipelines);
}

static Widget* addWidget(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, 
        const Obdn_U_ResponderFn inputHanderFn, 
        const Obdn_U_DrawFn      drawFn,
        const Obdn_U_DestructFn  destructFn,
        Widget* parent)
{
    Widget* widget = allocWidget();
    *widget = (Widget){
        .x = x, .y = y,
        .width = width,
        .height = height,
        .inputHandlerFn = inputHanderFn,
        .drawFn = drawFn,
        .destructFn = destructFn };

    if (!parent && rootWidget) // if no parent given and we have root widget, make widget its child
        parent = rootWidget;

    if (parent)
    {
        parent->widgets[parent->widgetCount++] = widget;
        widget->parent = parent;
        assert(parent->widgetCount < MAX_WIDGETS_PER_WIDGET);
    }

    return widget;
}

static bool clickTest(int16_t x, int16_t y, Widget* widget)
{
    x -= widget->x;
    y -= widget->y;
    bool xtest = (x > 0 && x < widget->width);
    bool ytest = (y > 0 && y < widget->height);
    return (xtest && ytest);
}

static void updateWidgetPos(const int16_t dx, const int16_t dy, Widget* widget)
{
    widget->x += dx;
    widget->y += dy;
    for (int i = 0; i < widget->primCount; i++) 
    {
        Obdn_R_Primitive* prim = &widget->primitives[i];
        Vec3* pos = obdn_r_GetPrimAttribute(prim, 0);
        for (int i = 0; i < prim->vertexCount; i++) 
        {
            pos[i].i += dx;
            pos[i].j += dy;
        }
    }

    // update children
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = widgetCount - 1; i >= 0; i--) 
    {
        updateWidgetPos(dx, dy, widget->widgets[i]);
    }
}

static bool propogateEventToChildren(const Hell_I_Event* event, Widget* widget)
{
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = widgetCount - 1; i >= 0 ; i--) 
    {
        if (widget->widgets[i]->inputHandlerFn)
        {
            const bool r = widget->widgets[i]->inputHandlerFn(event, widget->widgets[i]);
            if (r) return true;
        }
    }
    return false;
}

static void dfnSimpleBox(const VkCommandBuffer cmdBuf, const Widget* widget)
{
    assert(widget->primCount == 1);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[PIPELINE_BOX]);

    const Obdn_R_Primitive* prim = &widget->primitives[0];

    PushConstantFrag pc = {
        .i0 = widget->width,
        .i1 = widget->height
    };

    vkCmdPushConstants(cmdBuf, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(pc), &pc);

    obdn_r_DrawPrim(cmdBuf, prim);
}

static bool rfnSimpleBox(const Hell_I_Event* event, Widget* widget)
{
    if (propogateEventToChildren(event, widget)) return true;

    switch (event->type)
    {
        case HELL_I_MOUSEDOWN: 
        {
            const int16_t mx = event->data.mouseData.x;
            const int16_t my = event->data.mouseData.y;
            const bool r = clickTest(mx, my, widget);
            if (!r) return false;
            widget->dragData.active = r;
            widget->dragData.prevX = mx;
            widget->dragData.prevY = my;
            return true;
        }
        case HELL_I_MOUSEUP: widget->dragData.active = false; break;
        case HELL_I_MOTION: 
        {
            if (!widget->dragData.active) return false;
            const int16_t mx = event->data.mouseData.x;
            const int16_t my = event->data.mouseData.y;
            const int16_t dx = mx - widget->dragData.prevX;
            const int16_t dy = my - widget->dragData.prevY;
            widget->dragData.prevX = mx;
            widget->dragData.prevY = my;
            updateWidgetPos(dx, dy, widget);
            return true;
        }
        default: break;
    }
    return false;
}

static void dfnSlider(const VkCommandBuffer cmdBuf, const Widget* widget)
{
    assert(widget->primCount == 1);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[PIPELINE_SLIDER]);

    const Obdn_R_Primitive* prim = &widget->primitives[0];

    PushConstantFrag pc = {
        .i0 = widget->width,
        .i1 = widget->height,
        .f0 = widget->data.slider.sliderPos
    };

    vkCmdPushConstants(cmdBuf, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(pc), &pc);

    obdn_r_DrawPrim(cmdBuf, prim);
}

static bool rfnSlider(const Hell_I_Event* event, Widget* widget)
{
    switch (event->type)
    {
        case HELL_I_MOUSEDOWN: 
        {
            const int16_t mx = event->data.mouseData.x;
            const int16_t my = event->data.mouseData.y;
            const bool r = clickTest(mx, my, widget);
            if (!r) return false;
            widget->dragData.active = r;
            widget->data.slider.sliderPos = (float)(mx - widget->x) / widget->width;
            return true;
        }
        case HELL_I_MOUSEUP: widget->dragData.active = false; break;
        case HELL_I_MOTION: 
        {
            if (!widget->dragData.active) return false;
            const int16_t mx = event->data.mouseData.x;
            widget->data.slider.sliderPos = (float)(mx - widget->x) / widget->width;
            return true;
        }
        default: break;
    }
    return false;
}

static void dfnText(const VkCommandBuffer cmdBuf, const Widget* widget)
{
    assert(widget->primCount == 1);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[PIPELINE_TEXTURE]);

    const Obdn_R_Primitive* prim = &widget->primitives[0];

    PushConstantFrag pc = {
        .i0 = widget->data.text.imageIndex
    };

    vkCmdPushConstants(cmdBuf, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(pc), &pc);

    obdn_r_DrawPrim(cmdBuf, prim);
}

static void destructText(Widget* text)
{
    obdn_v_FreeImage(&images[text->data.text.imageIndex]);
}

static bool rfnPassThrough(const Hell_I_Event* event, Widget* widget)
{
    return propogateEventToChildren(event, widget);
}

static bool responder(const Hell_I_Event* event)
{
    return rootWidget->inputHandlerFn(event, rootWidget);
}

static void initFrameBuffers(uint32_t width, uint32_t height)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        const Obdn_R_Frame* frame = obdn_v_GetFrame(i);

        const VkImageView attachments[] = {
            frame->view
        };

        const VkFramebufferCreateInfo fbi = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width  = width,
            .height = height,
            .layers = 1,
        };

        V_ASSERT( vkCreateFramebuffer(device, &fbi, NULL, &framebuffers[i]) );
    }
}

static void destroyPipelines(void)
{
    for (int i = 0; i < PIPELINE_COUNT; i++) 
    {
        vkDestroyPipeline(device, pipelines[i], NULL);
    }
}

static void destroyFramebuffers(void)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        vkDestroyFramebuffer(device, framebuffers[i], NULL);   
    }
}

static void onSwapchainRecreate(void)
{
    VkExtent2D ex = obdn_v_GetSwapExtent();
    destroyPipelines();
    destroyFramebuffers();
    initPipelines(ex.width, ex.height);
    initFrameBuffers(ex.width, ex.height);
}

// does not modify parent
static void destroyWidget(Widget* widget)
{
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = 0; i < widgetCount; i++)
    {
        destroyWidget(widget->widgets[i]); // destroy children. careful, this can modify widgetCount
    }
    for (int i = 0; i < widget->primCount; i++)
    {
        obdn_r_FreePrim(&widget->primitives[i]);
    }
    if (widget->destructFn)
        widget->destructFn(widget);
    freeWidget(widget);
}

static int requestImageIndex(void)
{
    for (int i = 0; i < MAX_IMAGE_COUNT; i++)
    {
        if (images[i].size == 0) // available
            return i;
    }
    return -1;
}

void obdn_u_Init(const VkImageLayout inputLayout, const VkImageLayout finalLayout)
{
    VkExtent2D ex = obdn_v_GetSwapExtent();
    initRenderCommands();
    initRenderPass(inputLayout, finalLayout);
    initDescriptionsAndPipelineLayouts();
    initPipelines(ex.width, ex.height);
    initFrameBuffers(ex.width, ex.height);

    rootWidget = addWidget(0, 0, ex.width, ex.height, rfnPassThrough, NULL, NULL, NULL);

    hell_i_Subscribe(responder, HELL_I_MOUSE_BIT | HELL_I_KEY_BIT);
    obdn_v_RegisterSwapchainRecreationFn(onSwapchainRecreate);
    printf("Obdn UI initialized.\n");
}

Obdn_U_Widget* obdn_u_CreateSimpleBox(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, Widget* parent)
{
    Widget* widget = addWidget(x, y, width, height, rfnSimpleBox, dfnSimpleBox, NULL, parent);

    widget->primCount = 1;
    widget->primitives[0] = obdn_r_CreateQuadNDC(widget->x, widget->y, widget->width, widget->height);

    return widget;
}

Obdn_U_Widget* obdn_u_CreateSlider(const int16_t x, const int16_t y, 
        Widget* parent)
{
    Widget* widget = addWidget(x, y, 300, 40, rfnSlider, dfnSlider, NULL, parent);
    printf("Slider X %d Y %d\n", x, y);

    widget->primCount = 1;
    widget->primitives[0] = obdn_r_CreateQuadNDC(widget->x, widget->y, widget->width, widget->height);

    widget->data.slider.sliderPos = 0.5;

    return widget;
}

Obdn_U_Widget* obdn_u_CreateText(const int16_t x, const int16_t y, const char* text,
        Widget* parent)
{
    const int charCount = strlen(text);
    const int width = charCount * 25;
    Widget* widget = addWidget(x, y, width, 100, NULL, dfnText, destructText, parent);
    printf("UI: Text widget X %d Y %d\n", x, y);

    widget->primCount = 1;
    widget->primitives[0] = obdn_r_CreateQuadNDC(widget->x, widget->y, widget->width, widget->height);
    strncpy(widget->data.text.text, text, OBDN_U_MAX_TEXT_SIZE);

    const int imgId = requestImageIndex();
    assert(imgId != -1 && "No image available");
    images[imgId] = obdn_t_CreateTextImage(width, 100, 0,
                               50, 36, widget->data.text.text);
    widget->data.text.imageIndex = imgId;
    updateTexture(imgId);

    return widget;
}

void obdn_u_UpdateText(const char* text, Widget* widget)
{
    vkDeviceWaitIdle(device);
    obdn_t_UpdateTextImage(0, 50, text, &images[widget->data.text.imageIndex]);
    updateTexture(widget->data.text.imageIndex);
}

static void widgetReport(Widget* widget)
{
    printf("Widget %p:\n", widget);
    printf("\tPos: %d, %d\n", widget->x, widget->y);
    printf("\tDim: %d, %d\n", widget->width, widget->height);
    printf("\tChildCount: %d\n", widget->widgetCount);
    for (int i = 0; i < widget->widgetCount; i++)
    {
        widgetReport(widget->widgets[i]);
    }
}

void obdn_u_DebugReport(void)
{
    printf("========== Obdn_U_Report ==========\n");
    widgetReport(rootWidget);
}

static void drawWidget(const VkCommandBuffer cmdBuf, Widget* widget)
{
    for (int i = 0; i < widget->widgetCount; i++)
    {
        drawWidget(cmdBuf, widget->widgets[i]);
    }
    if (widget->drawFn)
        widget->drawFn(cmdBuf, widget);
}

VkSemaphore obdn_u_Render(const VkSemaphore waitSemephore)
{
    uint32_t frameIndex = obdn_v_GetCurrentFrameIndex();

    vkWaitForFences(device, 1, &renderCommands[frameIndex].fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &renderCommands[frameIndex].fence);

    vkResetCommandPool(device, renderCommands[frameIndex].pool, 0);

    VkCommandBufferBeginInfo cbbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    V_ASSERT( vkBeginCommandBuffer(renderCommands[frameIndex].buffer, &cbbi) );

    VkCommandBuffer cmdBuf = renderCommands[frameIndex].buffer;

    VkClearValue clear = {0};

    VkExtent2D ex = obdn_v_GetSwapExtent();
    
    const VkRenderPassBeginInfo rpassInfoUi = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 1,
        .pClearValues    = &clear,
        .renderArea      = {{0, 0}, {ex.width, ex.height}},
        .renderPass      = renderPass,
        .framebuffer     = framebuffers[frameIndex],
    };

    vkCmdBeginRenderPass(cmdBuf, &rpassInfoUi, VK_SUBPASS_CONTENTS_INLINE);

    PushConstantVert pc = {
        .i0 = ex.width,
        .i1 = ex.height 
    };

    vkCmdPushConstants(cmdBuf, pipelineLayout, 
            VK_SHADER_STAGE_VERTEX_BIT, 
            sizeof(PushConstantFrag), sizeof(PushConstantVert), &pc);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0,
                            descriptions[frameIndex].descriptorSetCount,
                            descriptions[frameIndex].descriptorSets, 0, NULL);

    drawWidget(cmdBuf, rootWidget);

    vkCmdEndRenderPass(cmdBuf);

    V_ASSERT( vkEndCommandBuffer(renderCommands[frameIndex].buffer) );
    
    obdn_v_SubmitGraphicsCommand(0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
            waitSemephore, renderCommands[frameIndex].semaphore,
            renderCommands[frameIndex].fence, renderCommands[frameIndex].buffer);

    return renderCommands[frameIndex].semaphore;
}

// does modify parent
void obdn_u_DestroyWidget(Widget* widget)
{
    int wi = -1;
    Widget* parent = widget->parent;
    if (!parent)
        parent = rootWidget;
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
    else {
        assert(0 && "Widget is not a child of its own parent...");
    }
}

void obdn_u_CleanUp(void)
{
    destroyFramebuffers();
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    destroyPipelines();
    vkDestroyRenderPass(device, renderPass, NULL);
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        obdn_v_DestroyCommand(renderCommands[i]);
        obdn_r_DestroyDescription(&descriptions[i]);
    }
    for (int i = 0; i < MAX_IMAGE_COUNT; i++)
    {
        if (images[i].size != 0)
            obdn_v_FreeImage(&images[i]);
    }
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
    destroyWidget(rootWidget);
    printf("Obsidian ui cleaned up.\n");
}

const VkSemaphore obdn_u_GetSemaphore(uint32_t frameIndex)
{
    return renderCommands[frameIndex].semaphore;
}

const VkFence obdn_u_GetFence(uint32_t frameIndex)
{
    return renderCommands[frameIndex].fence;
}
