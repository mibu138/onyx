#include "t_viewer.h"

#include "m_math.h"
#include "utils.h"
#include "def.h"
#include "d_display.h"
#include "v_memory.h"
#include "v_video.h"
#include "r_render.h"
#include "r_commands.h"
#include "r_raytrace.h"
#include "r_geo.h"
#include "i_input.h"
#include "g_game.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

jmp_buf exit_game;

#define NS_TARGET 16666666 // 1 / 60 seconds
//#define NS_TARGET 500000000
#define NS_PER_S  1000000000

const VkInstance* t_InitVulkan(void)
{
    return v_Init();
    printf("Video initialized\n");
}

void t_InitVulkanSwapchain(VkSurfaceKHR* surface)
{
    v_InitSwapchain(surface);
    printf("Swapchain initialized\n");
}


void t_StartViewer(void)
{
    r_Init();
    printf("Renderer initialized\n");
    i_Init();
    printf("Input initialized\n");

    struct timespec startTime = {0, 0};
    struct timespec endTime = {0, 0};
    struct timespec diffTime = {0, 0};
    struct timespec remTime = {0, 0}; // this is just if we get signal interupted

    uint64_t frameCount   = 0;
    uint64_t nsTotal      = 0;
    unsigned long nsDelta = 0;
    uint32_t shortestFrame = NS_PER_S;
    uint32_t longestFrame = 0;

    //CHECK( HAPI_CreateThriftSocketSession(&session, "boswell", 4000) );
    //
    Mesh mesh = r_CreateCube();

    r_LoadMesh(&mesh);

#if RAY_TRACE
    parms.mode = MODE_RAY;
    r_BuildBlas(&mesh);
    r_BuildTlas();
    r_CreateShaderBindingTable();
#endif

    //return 0;

    r_InitRenderCommands();
    g_Init();
    printf("Viewer initialized\n");

    //Mat4  m;
    //m = m_Ident_Mat4();
    //m = m_Translate_Mat4((Vec3){0.0, -2.5, -5}, &m);
    //m_ScaleUniform_Mat4(0.9, &m);
    //Mat4* xformModel = r_GetXform(R_XFORM_MODEL);
    //*xformModel = m;
    Mat4* xformProj  = r_GetXform(R_XFORM_PROJ);
    *xformProj = m_BuildPerspective(0.1, 30);

    Mat4* xformProjInv = r_GetXform(R_XFORM_PROJ_INV);

    printf("Projection matrix: \n");
    printMat4(xformProj);

    *xformProjInv = m_Invert4x4(xformProj);
    printf("Inverse projection matrix: \n");
    printMat4(xformProjInv);

    printf("Proj * Proj_inv: \n");
    Mat4 pxpi = m_Mult_Mat4(xformProj, xformProjInv);
    printMat4(&pxpi);

    parms.shouldRun = true;

    while( parms.shouldRun ) 
    {
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        i_GetEvents();
        i_ProcessEvents();

        //r_WaitOnQueueSubmit(); // possibly don't need this due to render pass

        g_Update();

        if (parms.renderNeedsUpdate) 
        {
            for (int i = 0; i < FRAME_COUNT; i++) 
            {
                r_WaitOnQueueSubmit();
                r_RequestFrame();
                r_UpdateRenderCommands();
                r_PresentFrame();
            }
            parms.renderNeedsUpdate = false;
        }

        r_RequestFrame();
        r_PresentFrame();

        clock_gettime(CLOCK_MONOTONIC, &endTime);

        nsDelta  = (endTime.tv_sec * NS_PER_S + endTime.tv_nsec) - (startTime.tv_sec * NS_PER_S + startTime.tv_nsec);
        nsTotal += nsDelta;

        printf("Delta ns: %ld\n", nsDelta);

        if (nsDelta > longestFrame) longestFrame = nsDelta;
        if (nsDelta < shortestFrame) shortestFrame = nsDelta;

        diffTime.tv_nsec = NS_TARGET > nsDelta ? NS_TARGET - nsDelta : 0;

        //assert ( NS_TARGET > nsDelta );

        nanosleep(&diffTime, &remTime);

        frameCount++;
    }

    vkDeviceWaitIdle(device);

    r_CommandCleanUp();
    i_CleanUp();
    r_CleanUp();
}

void t_CleanUp(void)
{
    v_CleanUp();
}
