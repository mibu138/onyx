#include "t_utils.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void printVec2(const Vec2* vec)
{
    printf("x: %f, y: %f\n", vec->x, vec->y);
}

void printVec3(const Vec3* vec)
{
    printf("x: %f, y: %f z: %f\n", vec->x[0], vec->x[1], vec->x[2]);
}


void printMat4(const Mat4 * m)
{
    for (int i = 0; i < 4; i++) 
    {
        for (int j = 0; j < 4; j++) 
        {
            printf("%f ", m->x[i][j]);
        }
        printf("\n");
    }
}

//void bitprintRowLen(const void *const thing, const size_t bitcount, const size_t rowlength)
//{
//    int mask;
//    for (int i = bitcount - 1; i >= 0; i--) {
//        mask = 1 << i;   
//        if (mask & *(uint64_t*)thing)
//            putchar('1');
//        else
//            putchar('0');
//    }
//    putchar('\n');
//}

void bytePrint(const void* const thing, const size_t byteCount)
{
    int mask;
    const uint8_t* base = (uint8_t*)thing;
    for (int i = byteCount - 1; i >= 0; i--) 
    {
        for (int j = 8 - 1; j >= 0; j--) 
        {
            mask = 1 << j;
            if (mask & *(base + i))
                putchar('1');
            else
                putchar('0');
        }
    }
    putchar('\n');
}

void bitprint(const void* const thing, const size_t bitcount)
{
    int mask;
    for (int i = bitcount - 1; i >= 0; i--) {
        mask = 1 << i;   
        if (mask & *(int*)thing)
            putchar('1');
        else
            putchar('0');
    }
    putchar('\n');
}

void timerStart(Tanto_Timer* t)
{
    clock_gettime(t->clockId, &t->startTime);
}

void timerStop(Tanto_Timer* t)
{
    clock_gettime(t->clockId, &t->endTime);
}

void tanto_TimerInit(Tanto_Timer* t)
{
    memset(t, 0, sizeof(Tanto_Timer));
    t->clockId = CLOCK_MONOTONIC;
}

void tanto_LoopStatsInit(Tanto_LoopStats* stats)
{
    memset(stats, 0, sizeof(Tanto_LoopStats));
    stats->longestFrame = UINT32_MAX;
}

void tanto_LoopStatsUpdate(const Tanto_Timer* t, Tanto_LoopStats* s)
{
    s->nsDelta  = (t->endTime.tv_sec * 1000000000 + t->endTime.tv_nsec) - (t->startTime.tv_sec * 1000000000 + t->startTime.tv_nsec);
    s->nsTotal += s->nsDelta;

    if (s->nsDelta > s->longestFrame) s->longestFrame = s->nsDelta;
    if (s->nsDelta < s->shortestFrame) s->shortestFrame = s->nsDelta;

    s->frameCount++;
}

void tanto_LoopSleep(const Tanto_LoopStats* s, const uint32_t nsTarget)
{
    struct timespec diffTime;
    diffTime.tv_nsec = nsTarget > s->nsDelta ? nsTarget - s->nsDelta : 0;
    diffTime.tv_sec  = 0;
    // we could use the second parameter to handle interrupts and signals
    nanosleep(&diffTime, NULL);
}
