#include "onyx/onyx.h"
#include <coal/coal.h>
#include <hell/cmd.h>
#include <hell/common.h>
#include <hell/debug.h>
#include <hell/len.h>
#include <hell/window.h>

#define WIREFRAME 0

static Hell_EventQueue* eventQueue;
static Hell_Grimoire*   grimoire;
static Hell_Mouth*  hellmouth;
static Hell_Console*    console;
static Hell_Window*     window;

static Onyx_Geometry triangle;

static VkFramebuffer    framebuffer;
static VkRenderPass     renderPass;
static VkPipelineLayout pipelineLayout;
static VkPipeline       pipeline;
static VkFramebuffer    framebuffers[2];
static Onyx_Image       depthImage;

static VkFence      drawFence;
static VkSemaphore  acquireSemaphore;
static VkSemaphore  drawSemaphore;
static Onyx_Command drawCommands[2];

static Onyx_Instance*  oInstance;
static Onyx_Memory*    oMemory;
static Onyx_Swapchain* swapchain;
static VkDevice        device;

static const VkFormat depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

static const char* vertex_shader_code =
    "layout(location = 0) in vec2 pos;"
    "void main()"
    "{"
    "   gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);"
    "}"
    "";

static const char* fragment_shader_code =
    "layout(location = 0) out vec4 out_color;"
    "void main()"
    "{"
    "   out_color = vec4(1, 0, 1, 1);\n"
    "}"
    "";

static void
createSurfaceDependent(void)
{
    VkExtent2D dim = onyx_GetSwapchainExtent(swapchain);
    depthImage     = onyx_CreateImage(
            oMemory, dim.width, dim.height, depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_SAMPLE_COUNT_1_BIT, 1, ONYX_MEMORY_DEVICE_TYPE);
    VkImageView attachments_0[2] = {onyx_GetSwapchainImageView(swapchain, 0),
                                    depthImage.view};
    VkImageView attachments_1[2] = {onyx_GetSwapchainImageView(swapchain, 1),
                                    depthImage.view};
    onyx_CreateFramebuffer(device, 2, attachments_0, dim.width, dim.height,
                           renderPass, &framebuffers[0]);
    onyx_CreateFramebuffer(device, 2, attachments_1, dim.width, dim.height,
                           renderPass, &framebuffers[1]);
}

static void
destroySurfaceDependent(void)
{
    onyx_FreeImage(&depthImage);
    onyx_DestroyFramebuffer(device, framebuffers[0]);
    onyx_DestroyFramebuffer(device, framebuffers[1]);
}

static void
onSwapchainRecreate(void)
{
    destroySurfaceDependent();
    createSurfaceDependent();
}

void
initApp(void)
{
    uint8_t attrSize = 3 * sizeof(float);
    triangle         = onyx_CreateGeometry(oMemory, 0x0, 3, 4, 1, &attrSize);
    Coal_Vec3* verts = (Coal_Vec3*)triangle.vertexRegion.hostData;
    verts[0].x       = 0.0;
    verts[0].y       = -1.0;
    verts[0].z       = 0.0;
    verts[1].x       = -1.0;
    verts[1].y       = 1.0;
    verts[1].z       = 0.0;
    verts[2].x       = 1.0;
    verts[2].y       = 1.0;
    verts[2].z       = 0.0;
    Onyx_AttrIndex* indices = (Onyx_AttrIndex*)triangle.indexRegion.hostData;
    indices[0]              = 0;
    indices[1]              = 1;
    indices[2]              = 2;
    indices[3] =
        0; // this last index is so we render the full triangle in line mode

    onyx_PrintGeo(&triangle);

    // call this render pass joe
    onyx_CreateRenderPass_ColorDepth(
        device, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        onyx_GetSwapchainFormat(swapchain), depthFormat, &renderPass);
    Onyx_PipelineLayoutInfo pli = {0}; // nothin
    onyx_CreatePipelineLayouts(device, 1, &pli, &pipelineLayout);
    
    VkShaderModule vertsm, fragsm;

    onyx_create_shader_module(device, &(SpirvCompileInfo){
            .entry_point = "main",
            .name = "vert",
            .shader_string = vertex_shader_code,
            .shader_type = ONYX_SHADER_TYPE_VERTEX
            }, &vertsm);

    onyx_create_shader_module(device, &(SpirvCompileInfo){
            .entry_point = "main",
            .name = "frag",
            .shader_string = fragment_shader_code,
            .shader_type = ONYX_SHADER_TYPE_FRAGMENT,
            }, &fragsm);

    OnyxGraphicsPipelineInfo pi = {
        .attachment_count = 1,
        .attachment_blends = &(OnyxPipelineColorBlendAttachment)
        {
            .blend_enable = true,
            .blend_mode = ONYX_BLEND_MODE_OVER
        },
        .cull_mode = VK_CULL_MODE_NONE,
        .depth_test_enable = false,
        .depth_write_enable = false,
        .device = device,
        .dynamic_state_count = 2,
        .dynamic_states = (VkDynamicState[2]){VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR},
        .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .layout = pipelineLayout,
        .polygon_mode = VK_POLYGON_MODE_FILL,
        .rasterization_samples = VK_SAMPLE_COUNT_1_BIT,
        .render_pass = renderPass,
        .shader_stage_count = 2,
        .shader_stages = (OnyxPipelineShaderStageInfo[2]){
            (OnyxPipelineShaderStageInfo){
                .module = vertsm,
                .entry_point = "main",
                .stage = VK_SHADER_STAGE_VERTEX_BIT
            },(OnyxPipelineShaderStageInfo){
                .module = fragsm,
                .entry_point = "main",
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT
            }
        },
        .subpass = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .vertex_attribute_description_count = 1,
        .vertex_attribute_descriptions = &(VkVertexInputAttributeDescription){
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .location = 0,
            .offset = 0,
        },
        .vertex_binding_description_count = 1,
        .vertex_binding_descriptions = &(VkVertexInputBindingDescription){
            .binding = 0,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            .stride = 12
        },
    };

    onyx_create_graphics_pipeline(&pi, NULL, &pipeline);

    createSurfaceDependent();
    onyx_CreateFence(device, &drawFence);
    onyx_CreateSemaphore(device, &drawSemaphore);
    onyx_CreateSemaphore(device, &acquireSemaphore);
    drawCommands[0] = onyx_CreateCommand(oInstance, ONYX_V_QUEUE_GRAPHICS_TYPE);
    drawCommands[1] = onyx_CreateCommand(oInstance, ONYX_V_QUEUE_GRAPHICS_TYPE);
}

#define TARGET_RENDER_INTERVAL 10000 // render every 30 ms

void
draw(i64 fi, i64 dt)
{
    static Hell_Tick timeOfLastRender    = 0;
    static Hell_Tick timeSinceLastRender = TARGET_RENDER_INTERVAL;
    static uint64_t  frameCounter        = 0;
    timeSinceLastRender                  = hell_Time() - timeOfLastRender;
    if (timeSinceLastRender < TARGET_RENDER_INTERVAL)
        return;
    timeOfLastRender    = hell_Time();
    timeSinceLastRender = 0;

    const Onyx_Frame* fb = onyx_AcquireSwapchainFrame(swapchain, VK_NULL_HANDLE,
                                                      acquireSemaphore);

    if (fb->dirty)
        onSwapchainRecreate();

    Onyx_Command cmd = drawCommands[frameCounter % 2];

    onyx_ResetCommand(&cmd);

    onyx_BeginCommandBuffer(cmd.buffer);

    const VkExtent2D dim = onyx_GetSwapchainExtent(swapchain);
    onyx_CmdSetViewportScissorFull(cmd.buffer, dim.width, dim.height);

    onyx_CmdBeginRenderPass_ColorDepth(cmd.buffer, renderPass,
                                       framebuffers[fb->index], dim.width,
                                       dim.height, 0.0, 0.0, 0.01, 1.0);

    vkCmdBindPipeline(cmd.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    onyx_DrawGeo(cmd.buffer, &triangle);

    onyx_CmdEndRenderPass(cmd.buffer);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitGraphicsCommand(
        oInstance, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 1,
        &acquireSemaphore, 1, &drawSemaphore, drawFence, cmd.buffer);

    bool result = onyx_PresentFrame(swapchain, 1, &drawSemaphore);

    onyx_WaitForFence(device, &drawFence);

    frameCounter++;
}

int
hellmain()
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
    hell_CreateHellmouth(grimoire, eventQueue, console, 1, &window, draw, NULL,
                         hellmouth);
    hell_SetVar(grimoire, "maxFps", 1000.0, 0);
    hell_Print("Starting hello triangle.\n");

    oInstance             = onyx_AllocInstance();
    oMemory               = onyx_AllocMemory();
    swapchain             = onyx_AllocSwapchain();
    Onyx_AovInfo depthAov = {
        .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
        .usageFlags  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .format      = depthFormat,
    };
#if UNIX
    const char* instanceExtensions[] = {VK_KHR_SURFACE_EXTENSION_NAME,
                                        VK_KHR_XCB_SURFACE_EXTENSION_NAME};
#elif WIN32
    const char* instanceExtensions[] = {VK_KHR_SURFACE_EXTENSION_NAME,
                                        VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
#endif
    Onyx_InstanceParms ip = {
        .enabledInstanceExentensionCount = LEN(instanceExtensions),
        .ppEnabledInstanceExtensionNames = instanceExtensions,
    };
#if WIN32
    onyx_SetRuntimeSpvPrefix("C:/dev/onyx/build/shaders/");
#endif
    onyx_CreateInstance(&ip, oInstance);
    onyx_CreateMemory(oInstance, 100, 100, 100, 0, 0, oMemory);
    onyx_CreateSwapchain(oInstance, oMemory, eventQueue, window,
                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1, &depthAov,
                         swapchain);
    device = onyx_GetDevice(oInstance);
    initApp();
    hell_Loop(hellmouth);
    return 0;
}

#ifdef WIN32
int APIENTRY
WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
        _In_ PSTR lpCmdLine, _In_ int nCmdShow)
{
    hell_SetHinstance(hInstance);
    hellmain();
    return 0;
}
#elif UNIX
int
main(int argc, char* argv[])
{
    hellmain();
}
#endif
