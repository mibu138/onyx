#include <hell/common.h>
#include <hell/debug.h>
#include <hell/window.h>
#include <hell/cmd.h>
#include <hell/len.h>
#include <coal/coal.h>
#include "obsidian/obsidian.h"
#include "obsidian/ui.h"

#define WIREFRAME 0

static Hell_EventQueue* eventQueue;
static Hell_Grimoire*   grimoire;
static Hell_Hellmouth*  hellmouth;
static Hell_Console*    console;
static Hell_Window*     window;

static Obdn_Geometry    triangle;

static VkFramebuffer       framebuffer;
static VkRenderPass        renderPass;
static VkPipelineLayout    pipelineLayout;
static VkPipeline          pipeline;
static VkFramebuffer       framebuffers[2];
static Obdn_Image          depthImage;

static VkFence      drawFence;
static VkSemaphore  acquireSemaphore;
static VkSemaphore  drawSemaphore;
static Obdn_Command drawCommands[2];

static Obdn_Swapchain* swapchain;

static const VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

static Obdn_Instance*  oInstance;
static Obdn_Memory*    oMemory;
static Obdn_Swapchain* swapchain;
static Obdn_UI*        ui;
static VkDevice        device;

static Obdn_U_Widget* uiBox;
static Obdn_U_Widget* uiText;

static void createSurfaceDependent(void)
{
    VkExtent2D dim = obdn_GetSwapchainExtent(swapchain);
    depthImage = obdn_CreateImage(oMemory, dim.width, dim.height,
                       depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                       VK_IMAGE_ASPECT_DEPTH_BIT, VK_SAMPLE_COUNT_1_BIT, 1,
                       OBDN_MEMORY_DEVICE_TYPE);
    VkImageView attachments_0[2] = {obdn_GetSwapchainImageView(swapchain, 0), depthImage.view};
    VkImageView attachments_1[2] = {obdn_GetSwapchainImageView(swapchain, 1), depthImage.view};
    obdn_CreateFramebuffer(device, 2, attachments_0, dim.width, dim.height, renderPass, &framebuffers[0]);
    obdn_CreateFramebuffer(device, 2, attachments_1, dim.width, dim.height, renderPass, &framebuffers[1]);

    // ui recreation
    VkImageView views[2] = {obdn_GetSwapchainImageView(swapchain, 0), obdn_GetSwapchainImageView(swapchain, 1)};
    obdn_RecreateSwapchainDependentUI(ui, dim.width, dim.height, 2, views);
}

static void destroySurfaceDependent(void)
{
    obdn_FreeImage(&depthImage);
    obdn_DestroyFramebuffer(device, framebuffers[0]);
    obdn_DestroyFramebuffer(device, framebuffers[1]);
}

static void onSwapchainRecreate(void)
{
    destroySurfaceDependent();
    createSurfaceDependent();
}

void init(void)
{
    uint8_t attrSize = 3 * sizeof(float);
    triangle = obdn_CreateGeometry(oMemory, 0x0, 3, 4, 1, &attrSize);
    Coal_Vec3* verts = (Coal_Vec3*)triangle.vertexRegion.hostData;
    verts[0].x =  0.0;
    verts[0].y = -1.0;
    verts[0].z =  0.0;
    verts[1].x = -1.0;
    verts[1].y =  1.0;
    verts[1].z =  0.0;
    verts[2].x =  1.0;
    verts[2].y =  1.0;
    verts[2].z =  0.0;
    Obdn_AttrIndex* indices = (Obdn_AttrIndex*)triangle.indexRegion.hostData;
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 0; // this last index is so we render the full triangle in line mode

    obdn_PrintGeo(&triangle);

    // call this render pass joe
    obdn_CreateRenderPass_ColorDepth(device,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        obdn_GetSwapchainFormat(swapchain), depthFormat, &renderPass);
    Obdn_PipelineLayoutInfo pli = {0}; //nothin
    obdn_CreatePipelineLayouts(device, 1, &pli, &pipelineLayout);
    if (WIREFRAME)
        obdn_CreateGraphicsPipeline_Taurus(device, renderPass, pipelineLayout, VK_POLYGON_MODE_LINE, &pipeline);
    else
        obdn_CreateGraphicsPipeline_Taurus(device, renderPass, pipelineLayout, VK_POLYGON_MODE_FILL, &pipeline);

    createSurfaceDependent();
    obdn_CreateFence(device, &drawFence);
    obdn_CreateSemaphore(device, &drawSemaphore);
    obdn_CreateSemaphore(device, &acquireSemaphore);
    drawCommands[0] = obdn_CreateCommand(oInstance, OBDN_V_QUEUE_GRAPHICS_TYPE);
    drawCommands[1] = obdn_CreateCommand(oInstance, OBDN_V_QUEUE_GRAPHICS_TYPE);
}

#define TARGET_RENDER_INTERVAL 500

void draw(i64 fi, i64 dt)
{
    static Hell_Tick timeOfLastRender = 0;
    static Hell_Tick timeSinceLastRender = TARGET_RENDER_INTERVAL;
    static uint64_t frameCounter = 0;
    timeSinceLastRender = hell_Time() - timeOfLastRender;
    if (timeSinceLastRender < TARGET_RENDER_INTERVAL)
        return;
    timeOfLastRender = hell_Time();
    timeSinceLastRender = 0;

    const Obdn_Frame* fb = obdn_AcquireSwapchainFrame(swapchain, VK_NULL_HANDLE, acquireSemaphore);

    if (fb->dirty)
        onSwapchainRecreate();

    Obdn_Command cmd = drawCommands[frameCounter % 2];

    obdn_ResetCommand(&cmd);

    obdn_BeginCommandBuffer(cmd.buffer);

    const VkExtent2D dim = obdn_GetSwapchainExtent(swapchain);
    obdn_CmdSetViewportScissorFull(cmd.buffer, dim.width, dim.height);

    obdn_CmdBeginRenderPass_ColorDepth(
        cmd.buffer, renderPass, framebuffers[fb->index], dim.width,
        dim.height, 0.0, 0.0, 0.01, 1.0);

        vkCmdBindPipeline(cmd.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        obdn_DrawGeo(cmd.buffer, &triangle);

    obdn_CmdEndRenderPass(cmd.buffer);

    obdn_RenderUI(ui, fb->index, dim.width, dim.height, cmd.buffer);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitGraphicsCommand(oInstance, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 1,
            &acquireSemaphore, 1, &drawSemaphore, drawFence, cmd.buffer);


    bool result = obdn_PresentFrame(swapchain, 1, &drawSemaphore);

    obdn_WaitForFence(device, &drawFence);

    frameCounter++;
}

int main(int argc, char *argv[])
{
    eventQueue = hell_AllocEventQueue();
    grimoire   = hell_AllocGrimoire();
    console    = hell_AllocConsole();
    window     = hell_AllocWindow();
    hellmouth  = hell_AllocHellmouth();
    hell_CreateConsole(console);
    hell_CreateEventQueue(eventQueue);
    hell_CreateGrimoire(eventQueue, grimoire);
    hell_CreateWindow(eventQueue, 666, 666, 0, window);
    hell_CreateHellmouth(grimoire, eventQueue, console, 1, &window, draw, NULL, hellmouth);
    hell_SetVar(grimoire, "maxFps", 1000, 0);
    hell_Print("Starting hello triangle.\n");

    oInstance = obdn_AllocInstance();
    oMemory   = obdn_AllocMemory();
    swapchain = obdn_AllocSwapchain();
    ui        = obdn_AllocUI();
    #if UNIX
    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    };
    #elif WIN32
    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    #endif
    Obdn_InstanceParms ip = {
        .enabledInstanceExentensionCount = LEN(instanceExtensions),
        .ppEnabledInstanceExtensionNames = instanceExtensions
    };
    obdn_CreateInstance(&ip, oInstance);
    obdn_CreateMemory(oInstance, 100, 100, 100, 0, 0, oMemory);
    obdn_CreateSwapchain(oInstance, oMemory, eventQueue, window, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0, NULL, swapchain);
    device = obdn_GetDevice(oInstance);

    VkImageView views[2] = {obdn_GetSwapchainImageView(swapchain, 0), obdn_GetSwapchainImageView(swapchain, 1)};

    obdn_CreateUI(oMemory, eventQueue, window, obdn_GetSwapchainFormat(swapchain),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                2,
                views, ui);
    uiBox = obdn_CreateSimpleBoxWidget(ui, 10, 10, 200, 100, NULL);
    uiText = obdn_CreateTextWidget(ui, 40, 40, "Blumpkin", uiBox);
    init();
    hell_Loop(hellmouth);
    return 0;
}
