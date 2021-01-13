#include "u_ui.h"
#include "r_geo.h"
#include "r_render.h"
#include "r_renderpass.h"
#include "r_pipeline.h"
#include "t_def.h"
#include "i_input.h"
#include "tanto/v_command.h"
#include "v_video.h"
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#define MAX_WIDGETS 256 // might actually be a good design constraint

#define SPVDIR "./shaders/spv"

typedef Tanto_U_Widget Widget;

typedef struct {
    int16_t prevX;
    int16_t prevY;
    bool    active;
} DragData;

typedef struct {
    int   i0;
    int   i1;
    float f0;
    float f1;
} PushConstant;

static uint32_t widgetCount;
static Widget   widgets[MAX_WIDGETS];
static Widget*  rootWidget;

static DragData dragData[MAX_WIDGETS];

static VkRenderPass     renderPass;

static VkPipelineLayout pipelineLayouts[TANTO_MAX_PIPELINES];
static VkPipeline       pipelines[TANTO_MAX_PIPELINES];

static VkFramebuffer    framebuffers[TANTO_FRAME_COUNT];

static Tanto_V_Command  renderCommands[TANTO_FRAME_COUNT];

enum {
    PIPELINE_BOX,
    PIPELINE_SLIDER,
};

static void initRenderCommands(void)
{
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        renderCommands[i] = tanto_v_CreateCommand(TANTO_V_QUEUE_GRAPHICS_TYPE);
        printf("UI COMMAND BUF: %p\n", renderCommands[i].buffer);
    }
}

static void initRenderPass(VkImageLayout initialLayout)
{
    const VkAttachmentDescription attachmentColor = {
        .flags = 0,
        .format = tanto_r_GetSwapFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = initialLayout,
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

    tanto_r_CreateRenderPass(&rpi, &renderPass);
}

static void initPipelineLayouts(void)
{
    VkPushConstantRange pcRange = {
        .offset = 0,
        .size = sizeof(PushConstant),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    const Tanto_R_PipelineLayoutInfo pipelayoutInfos[] = {{
        .pushConstantCount = 1,
        .pushConstantsRanges = &pcRange
    }};

    tanto_r_CreatePipelineLayouts(TANTO_ARRAY_SIZE(pipelayoutInfos), 
            pipelayoutInfos, pipelineLayouts);
}

static void initPipelines(void)
{
    Tanto_R_GraphicsPipelineInfo pipeInfos[] = {{
        // simple box
        .renderPass = renderPass, 
        .layout     = pipelineLayouts[0],
        .sampleCount = VK_SAMPLE_COUNT_1_BIT,
        .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .blendMode   = TANTO_R_BLEND_MODE_OVER,
        .vertexDescription = tanto_r_GetVertexDescription3D_2Vec3(),
        .vertShader = TANTO_SPVDIR"/ui-vert.spv",
        .fragShader = TANTO_SPVDIR"/ui-box-frag.spv"
    },{ 
        // slider
        .renderPass = renderPass, 
        .layout     = pipelineLayouts[0],
        .sampleCount = VK_SAMPLE_COUNT_1_BIT,
        .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .blendMode   = TANTO_R_BLEND_MODE_OVER,
        .vertexDescription = tanto_r_GetVertexDescription3D_2Vec3(),
        .vertShader = TANTO_SPVDIR"/ui-vert.spv",
        .fragShader = TANTO_SPVDIR"/ui-slider-frag.spv"
    }};

    tanto_r_CreateGraphicsPipelines(TANTO_ARRAY_SIZE(pipeInfos), pipeInfos, pipelines);
}

static Widget* addWidget(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, 
        const Tanto_U_ResponderFn rfn, 
        const Tanto_U_DrawFn      dfn,
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
        Tanto_R_Primitive* prim = &widget->primitives[i];
        Tanto_R_Attribute* pos = tanto_r_GetPrimAttribute(prim, 0);
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

static void dfnSimpleBox(const VkCommandBuffer cmdBuf, const Widget* widget)
{
    assert(widget->primCount == 1);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[PIPELINE_BOX]);

    const Tanto_R_Primitive* prim = &widget->primitives[0];

    tanto_r_BindPrim(cmdBuf, prim);

    PushConstant pc = {
        .i0 = widget->width,
        .i1 = widget->height
    };

    vkCmdPushConstants(cmdBuf, pipelineLayouts[0], VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(pc), &pc);

    vkCmdDrawIndexed(cmdBuf, prim->indexCount, 1, 0, 0, 0);
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

    const Tanto_R_Primitive* prim = &widget->primitives[0];

    tanto_r_BindPrim(cmdBuf, prim);

    PushConstant pc = {
        .i0 = widget->width,
        .i1 = widget->height,
        .f0 = widget->data.slider.sliderPos
    };

    vkCmdPushConstants(cmdBuf, pipelineLayouts[0], VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, sizeof(pc), &pc);

    vkCmdDrawIndexed(cmdBuf, prim->indexCount, 1, 0, 0, 0);
}

static bool rfnSlider(const Tanto_I_Event* event, Widget* widget)
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
            widget->data.slider.sliderPos = (float)(mx - widget->x) / widget->width;
            return true;
        }
        case TANTO_I_MOUSEUP: dragData[id].active = false; break;
        case TANTO_I_MOTION: 
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

static bool rfnPassThrough(const Tanto_I_Event* event, Widget* widget)
{
    return propogateEventToChildren(event, widget);
}

static bool responder(const Tanto_I_Event* event)
{
    return widgets[0].inputHandlerFn(event, &widgets[0]);
}

static void initFrameBuffers(void)
{
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        Tanto_R_Frame* frame = tanto_r_GetFrame(i);

        const VkImageView attachments[] = {
            frame->swapImage.view
        };

        const VkFramebufferCreateInfo fbi = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = TANTO_WINDOW_WIDTH,
            .height = TANTO_WINDOW_HEIGHT,
            .layers = 1,
        };

        V_ASSERT( vkCreateFramebuffer(device, &fbi, NULL, &framebuffers[i]) );
    }
}

static void destroyPipelines(void)
{
    for (int i = 0; i < TANTO_MAX_PIPELINES; i++) 
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
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
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

void tanto_u_Init(const VkImageLayout inputLayout)
{
    initRenderCommands();
    initRenderPass(inputLayout);
    initPipelineLayouts();
    initPipelines();
    initFrameBuffers();

    rootWidget = addWidget(0, 0, TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT, rfnPassThrough, NULL, NULL);

    tanto_i_Subscribe(responder);
    tanto_r_RegisterSwapchainRecreationFn(onSwapchainRecreate);
    printf("Tanto UI initialized.\n");
}

Tanto_U_Widget* tanto_u_CreateSimpleBox(const int16_t x, const int16_t y, 
        const int16_t width, const int16_t height, Widget* parent)
{
    Widget* widget = addWidget(x, y, width, height, rfnSimpleBox, dfnSimpleBox, parent);

    widget->primCount = 1;
    widget->primitives[0] = tanto_r_CreateQuadNDC(widget->x, widget->y, widget->width, widget->height, NULL);

    return widget;
}

Tanto_U_Widget* tanto_u_CreateSlider(const int16_t x, const int16_t y, 
        Widget* parent)
{
    Widget* widget = addWidget(x, y, 300, 40, rfnSlider, dfnSlider, parent);

    widget->primCount = 1;
    widget->primitives[0] = tanto_r_CreateQuadNDC(widget->x, widget->y, widget->width, widget->height, NULL);

    widget->data.slider.sliderPos = 0.5;

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

void tanto_u_Render(void)
{
    uint32_t frameIndex = tanto_r_GetCurrentFrameIndex();

    vkWaitForFences(device, 1, &renderCommands[frameIndex].fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &renderCommands[frameIndex].fence);

    vkResetCommandPool(device, renderCommands[frameIndex].pool, 0);

    VkCommandBufferBeginInfo cbbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    V_ASSERT( vkBeginCommandBuffer(renderCommands[frameIndex].buffer, &cbbi) );

    const Tanto_U_Widget* iter = NULL;
    const uint8_t widgetCount = tanto_u_GetWidgets(&iter);

    VkCommandBuffer cmdBuf = renderCommands[frameIndex].buffer;

    VkClearValue clear = {0};
    
    const VkRenderPassBeginInfo rpassInfoUi = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .clearValueCount = 1,
        .pClearValues = &clear,
        .renderArea = {{0, 0}, {TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT}},
        .renderPass =  renderPass,
        .framebuffer = framebuffers[frameIndex],
    };

    vkCmdBeginRenderPass(cmdBuf, &rpassInfoUi, VK_SUBPASS_CONTENTS_INLINE);

    for (int w = 0; w < widgetCount; w++) 
    {
        const Tanto_U_Widget* widget = &(iter[w]);
        if (widget->drawFn)
            widget->drawFn(cmdBuf, widget);
    }
    vkCmdEndRenderPass(cmdBuf);

    V_ASSERT( vkEndCommandBuffer(renderCommands[frameIndex].buffer) );
    
    tanto_r_SubmitUI(renderCommands[frameIndex]);

    tanto_r_PresentFrame(&renderCommands[frameIndex].semaphore);
}

void tanto_u_CleanUp(void)
{
    destroyFramebuffers();
    for (int i = 0; i < TANTO_MAX_PIPELINES; i++) 
    {
        if (pipelineLayouts[i])
        {
            vkDestroyPipelineLayout(device, pipelineLayouts[i], NULL);
            pipelineLayouts[i] = 0;
        }
    }
    destroyPipelines();
    vkDestroyRenderPass(device, renderPass, NULL);
    for (int i = 0; i < TANTO_FRAME_COUNT; i++) 
    {
        tanto_v_DestroyCommand(renderCommands[i]);
    }
    rootWidget = NULL;
    widgetCount = 0;
}
