#include "m_math.h"
#include "utils.h"
#include "def.h"
#include "d_display.h"
#include "v_video.h"
#include "r_render.h"
#include "r_commands.h"
#include "i_input.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

jmp_buf exit_game;

#define NS_TARGET 16666666 // 1 / 60 seconds
//#define NS_TARGET 500000000
#define NS_PER_S  1000000000

int main(int argc, char *argv[])
{
    d_Init();
    printf("Display initialized\n");
    v_Init();
    printf("Video initialized\n");
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

    r_InitRenderCommands();

    float angle = 0.01;
    Mat4* xformModel = r_GetXform(R_XFORM_MODEL);
    Mat4  m;

    while( 1 ) 
    {
        if (setjmp(exit_game)) break;

        clock_gettime(CLOCK_MONOTONIC, &startTime);


        i_GetEvents();
        i_ProcessEvents();

        r_WaitOnQueueSubmit();

        angle += 0.01;

        m = m_Ident_Mat4();
        m = m_Translate_Mat4((Vec3){0.25, 0.00, 0.0}, &m);
        m = m_RotateZ_Mat4(angle, &m);
        m = m_RotateY_Mat4(angle, &m);
        m_ScaleUniform_Mat4(0.5, &m);
        *xformModel = m;

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

    r_WaitOnQueueSubmit();

    i_CleanUp();
    r_CleanUp();
    v_CleanUp();
    d_CleanUp();
    return 0;
}
