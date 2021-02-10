#include "u_ui.h"
#include "r_geo.h"
#include "r_render.h"
#include "r_renderpass.h"
#include "r_pipeline.h"
#include "t_def.h"
#include "i_input.h"
#include "v_command.h"
#include "v_video.h"
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#define MAX_WIDGETS 256 // might actually be a good design constraint

#define SPVDIR "./shaders/spv"

typedef Obdn_U_Widget Widget;

typedef struct {
    int16_t prevX;
    int16_t prevY;
    bool    active;
} DragData;

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

static uint32_t widgetCount;
static Widget   widgets[MAX_WIDGETS];
static Widget*  rootWidget;

static DragData dragData[MAX_WIDGETS];

static VkRenderPass     renderPass;

static VkPipelineLayout pipelineLayouts[OBDN_MAX_PIPELINES];
static VkPipeline       pipelines[OBDN_MAX_PIPELINES];

static VkFramebuffer    framebuffers[OBDN_FRAME_COUNT];

static Obdn_V_Command  renderCommands[OBDN_FRAME_COUNT];

enum {
    PIPELINE_BOX,
    PIPELINE_SLIDER,
};

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
    const VkAttachmentDescription attachmentColor = {
        .flags          = 0,
        .format         = obdn_r_GetSwapFormat(),
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = initialLayout,
        .finalLayout    = finalLayout,
    };

    const VkAttachmentDescription attachments[] = {
        attachmentColor
    };

    VkAttachmentReference colorReference = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

    Obdn_R_RenderPassInfo rpi = {
        .attachmentCount = 1,
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
    };

    obdn_r_CreateRenderPass(&rpi, &renderPass);
}

static void initPipelineLayouts(void)
{
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
        .pushConstantCount   = OBDN_ARRAY_SIZE(pcRanges),
        .pushConstantsRanges = pcRanges
    }};

    obdn_r_CreatePipelineLayouts(OBDN_ARRAY_SIZE(pipelayoutInfos), 
            pipelayoutInfos, pipelineLayouts);
}

static void initPipelines(void)
{
    Obdn_R_AttributeSize attrSizes[2] = {12, 12};
    Obdn_R_GraphicsPipelineInfo pipeInfos[] = {{
        // simple box
        .renderPass        = renderPass,
        .layout            = pipelineLayouts[0],
        .sampleCount       = VK_SAMPLE_COUNT_1_BIT,
        .frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .blendMode         = OBDN_R_BLEND_MODE_OVER,
        .vertexDescription = obdn_r_GetVertexDescription(2, attrSizes),
        .vertShader        = OBDN_SPVDIR"/ui-vert.spv",
        .fragShader        = OBDN_SPVDIR"/ui-box-frag.spv"
    },{ 
        // slider
        .renderPass        = renderPass,
        .layout            = pipelineLayouts[0],
        .sampleCount       = VK_SAMPLE_COUNT_1_BIT,
        .frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .blendMode         = OBDN_R_BLEND_MODE_OVER,
        .vertexDescription = obdn_r_GetVertexDescription(2, attrSizes),
        .vertShader        = OBDN_SPVDIR"/ui-vert.spv",
        .fragShader        = OBDN_SPVDIR"/ui-slider-frag.spv"
    }};

    obdn_r_CreateGraphicsPipelines(OBDN_ARRAY_SIZE(pipeInfos), pipeInfos, pipelines);
}

static Widget* addWidget(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, 
        const Obdn_U_ResponderFn rfn, 
        const Obdn_U_DrawFn      dfn,
        Widget* parent)
{
    widgets[widgetCount] = (Widget){
        .id = widgetCount,
        .x = x, .y = y,
        .width = width,
        .height = height,
        .inputHandlerFn = rfn,
        .drawFn = dfn
    };

    if (!parent && rootWidget) // if no parent given and we have root widget, make widget its child
        parent = rootWidget;

    if (parent)
        parent->widgets[parent->widgetCount++] = &widgets[widgetCount];

    return &widgets[widgetCount++];
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

static bool propogateEventToChildren(const Obdn_I_Event* event, Widget* widget)
{
    const uint8_t widgetCount = widget->widgetCount;
    for (int i = widgetCount - 1; i >= 0 ; i--) 
    {
        const bool r = widget->widgets[i]->inputHandlerFn(event, widget->widgets[i]);
        if (r) return true;
    }
    return false;
}

static void dfnSimpleBox(const VkCommandBuffer cmdBuf, const Widget* widget)
{
    assert(widget->primCount == 1);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[PIPELINE_BOX]);

    const Obdn_R_Primitive* prim = &widget->primitives[0];

    obdn_r_BindPrim(cmdBuf, prim);

    PushConstantFrag pc = {
        .i0 = widget->width,
        .i1 = widget->height
    };

    vkCmdPushConstants(cmdBuf, pipelineLayouts[0], VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(pc), &pc);

    vkCmdDrawIndexed(cmdBuf, prim->indexCount, 1, 0, 0, 0);
}

static bool rfnSimpleBox(const Obdn_I_Event* event, Widget* widget)
{
    if (propogateEventToChildren(event, widget)) return true;
    const uint8_t id = widget->id;

    switch (event->type)
    {
        case OBDN_I_MOUSEDOWN: 
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
        case OBDN_I_MOUSEUP: dragData[id].active = false; break;
        case OBDN_I_MOTION: 
        {
            if (!dragData[id].active) return false;
            const int16_t mx = event->data.mouseData.x;
            const int16_t my = event->data.mouseData.y;
            const int16_t dx = mx - dragData[id].prevX;
            const int16_t dy = my - dragData[id].prevY;
            dragData[id].prevX = mx;
            dragData[id].prevY = my;
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

    obdn_r_BindPrim(cmdBuf, prim);

    PushConstantFrag pc = {
        .i0 = widget->width,
        .i1 = widget->height,
        .f0 = widget->data.slider.sliderPos
    };

    vkCmdPushConstants(cmdBuf, pipelineLayouts[0], VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(pc), &pc);

    vkCmdDrawIndexed(cmdBuf, prim->indexCount, 1, 0, 0, 0);
}

static bool rfnSlider(const Obdn_I_Event* event, Widget* widget)
{
    const uint8_t id = widget->id;

    switch (event->type)
    {
        case OBDN_I_MOUSEDOWN: 
        {
            const int16_t mx = event->data.mouseData.x;
            const int16_t my = event->data.mouseData.y;
            const bool r = clickTest(mx, my, widget);
            if (!r) return false;
            dragData[id].active = r;
            widget->data.slider.sliderPos = (float)(mx - widget->x) / widget->width;
            return true;
        }
        case OBDN_I_MOUSEUP: dragData[id].active = false; break;
        case OBDN_I_MOTION: 
        {
            if (!dragData[id].active) return false;
            const int16_t mx = event->data.mouseData.x;
            widget->data.slider.sliderPos = (float)(mx - widget->x) / widget->width;
            return true;
        }
        default: break;
    }
    return false;
    return false;
}

static bool rfnPassThrough(const Obdn_I_Event* event, Widget* widget)
{
    return propogateEventToChildren(event, widget);
}

static bool responder(const Obdn_I_Event* event)
{
    return widgets[0].inputHandlerFn(event, &widgets[0]);
}

static void initFrameBuffers(void)
{
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        const Obdn_R_Frame* frame = obdn_r_GetFrame(i);

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
            .width = OBDN_WINDOW_WIDTH,
            .height = OBDN_WINDOW_HEIGHT,
            .layers = 1,
        };

        V_ASSERT( vkCreateFramebuffer(device, &fbi, NULL, &framebuffers[i]) );
    }
}

static void destroyPipelines(void)
{
    for (int i = 0; i < OBDN_MAX_PIPELINES; i++) 
    {
        if (pipelines[i])
        {
            vkDestroyPipeline(device, pipelines[i], NULL);
            pipelines[i] = 0;
        }
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
    destroyPipelines();
    destroyFramebuffers();
    initPipelines();
    initFrameBuffers();
}

void obdn_u_Init(const VkImageLayout inputLayout, const VkImageLayout finalLayout)
{
    initRenderCommands();
    initRenderPass(inputLayout, finalLayout);
    initPipelineLayouts();
    initPipelines();
    initFrameBuffers();

    rootWidget = addWidget(0, 0, OBDN_WINDOW_WIDTH, OBDN_WINDOW_HEIGHT, rfnPassThrough, NULL, NULL);

    obdn_i_Subscribe(responder);
    obdn_r_RegisterSwapchainRecreationFn(onSwapchainRecreate);
    printf("Obdn UI initialized.\n");
}

Obdn_U_Widget* obdn_u_CreateSimpleBox(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, Widget* parent)
{
    Widget* widget = addWidget(x, y, width, height, rfnSimpleBox, dfnSimpleBox, parent);

    widget->primCount = 1;
    widget->primitives[0] = obdn_r_CreateQuadNDC(widget->x, widget->y, widget->width, widget->height);

    return widget;
}

Obdn_U_Widget* obdn_u_CreateSlider(const int16_t x, const int16_t y, 
        Widget* parent)
{
    Widget* widget = addWidget(x, y, 300, 40, rfnSlider, dfnSlider, parent);
    printf("Slider X %d Y %d\n", x, y);

    widget->primCount = 1;
    widget->primitives[0] = obdn_r_CreateQuadNDC(widget->x, widget->y, widget->width, widget->height);

    widget->data.slider.sliderPos = 0.5;

    return widget;
}

void obdn_u_DebugReport(void)
{
    printf("========== Obdn_U_Report ==========\n");
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

uint8_t obdn_u_GetWidgets(const Obdn_U_Widget** pToFirst)
{
    *pToFirst = widgets;
    return widgetCount;
}

VkSemaphore obdn_u_Render(const VkSemaphore waitSemephore)
{
    uint32_t frameIndex = obdn_r_GetCurrentFrameIndex();

    vkWaitForFences(device, 1, &renderCommands[frameIndex].fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &renderCommands[frameIndex].fence);

    vkResetCommandPool(device, renderCommands[frameIndex].pool, 0);

    VkCommandBufferBeginInfo cbbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    V_ASSERT( vkBeginCommandBuffer(renderCommands[frameIndex].buffer, &cbbi) );

    const Obdn_U_Widget* iter = NULL;
    const uint8_t widgetCount = obdn_u_GetWidgets(&iter);

    VkCommandBuffer cmdBuf = renderCommands[frameIndex].buffer;

    VkClearValue clear = {0};
    
    const VkRenderPassBeginInfo rpassInfoUi = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 1,
        .pClearValues    = &clear,
        .renderArea      = {{0, 0}, {OBDN_WINDOW_WIDTH, OBDN_WINDOW_HEIGHT}},
        .renderPass      = renderPass,
        .framebuffer     = framebuffers[frameIndex],
    };

    vkCmdBeginRenderPass(cmdBuf, &rpassInfoUi, VK_SUBPASS_CONTENTS_INLINE);

    PushConstantVert pc = {
        .i0 = OBDN_WINDOW_WIDTH,
        .i1 = OBDN_WINDOW_HEIGHT
    };

    vkCmdPushConstants(cmdBuf, pipelineLayouts[0], 
            VK_SHADER_STAGE_VERTEX_BIT, 
            sizeof(PushConstantFrag), sizeof(PushConstantVert), &pc);

    for (int w = 0; w < widgetCount; w++) 
    {
        const Obdn_U_Widget* widget = &(iter[w]);
        if (widget->drawFn)
            widget->drawFn(cmdBuf, widget);
    }
    vkCmdEndRenderPass(cmdBuf);

    V_ASSERT( vkEndCommandBuffer(renderCommands[frameIndex].buffer) );
    
    obdn_v_SubmitGraphicsCommand(0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
            waitSemephore, renderCommands[frameIndex].semaphore,
            renderCommands[frameIndex].fence, renderCommands[frameIndex].buffer);

    return renderCommands[frameIndex].semaphore;
}

void obdn_u_DestroyWidget(Widget* widget)
{
    for (int i = 0; i < widget->widgetCount; i++)
    {
        obdn_u_DestroyWidget(widget->widgets[i]); // destroy children
    }
}

void obdn_u_CleanUp(void)
{
    destroyFramebuffers();
    for (int i = 0; i < OBDN_MAX_PIPELINES; i++) 
    {
        if (pipelineLayouts[i])
        {
            vkDestroyPipelineLayout(device, pipelineLayouts[i], NULL);
            pipelineLayouts[i] = 0;
        }
    }
    destroyPipelines();
    vkDestroyRenderPass(device, renderPass, NULL);
    for (int i = 0; i < OBDN_FRAME_COUNT; i++) 
    {
        obdn_v_DestroyCommand(renderCommands[i]);
    }
    rootWidget = NULL;
    widgetCount = 0;
}

const VkSemaphore obdn_u_GetSemaphore(uint32_t frameIndex)
{
    return renderCommands[frameIndex].semaphore;
}

const VkFence obdn_u_GetFence(uint32_t frameIndex)
{
    return renderCommands[frameIndex].fence;
}
