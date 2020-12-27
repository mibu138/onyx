#ifndef T_UTILS_H
#define T_UTILS_H

#include <stdio.h>
#include <coal/coal.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#define printBuffer(pBuffer, size, Type, printFn) { \
    const Type* util_pBuffer = (Type*)pBuffer; \
    for (int i = 0; i < size / sizeof(Type); i++) \
    { \
        printFn(&util_pBuffer[i]); \
    } \
} ;

void printVec2(const Vec2*);
void printVec3(const Vec3*);
void printVec4(const Vec4*);
void printMat4(const Mat4*);
void bitprintRowLen(const void *const thing, const size_t bitcount, const size_t rowlength);
void bytePrint(const void* const thing, const size_t byteCount);
void bitprint(const void *const thing, const size_t bitcount);

typedef struct Tanto_Timer Tanto_Timer;

struct Tanto_Timer {
    struct timespec startTime;
    struct timespec endTime;
    clockid_t clockId;
};

void tanto_TimerStart(Tanto_Timer* t);
void tanto_TimerStop(Tanto_Timer* t);
void tanto_TimerInit(Tanto_Timer* t);
void tanto_PrintTime(const Tanto_Timer* t);

typedef struct Tanto_LoopStats Tanto_LoopStats;

struct Tanto_LoopStats {
    uint64_t frameCount;
    uint64_t nsTotal;
    unsigned long nsDelta;
    uint32_t shortestFrame;
    uint32_t longestFrame;
};

void tanto_LoopStatsInit(Tanto_LoopStats* stats);
void tanto_LoopStatsUpdate(const Tanto_Timer* t, Tanto_LoopStats* s);
void tanto_LoopSleep(const Tanto_LoopStats* s, const uint32_t nsTarget);

typedef struct {
    Tanto_Timer     timer;
    Tanto_LoopStats loopStats;
    const uint32_t  targetNs;
    const bool      printFps;
    const bool      printNs;
} Tanto_LoopData;

Tanto_LoopData tanto_CreateLoopData(const uint32_t targetNs, const bool printFps, const bool printNS);
void tanto_FrameStart(Tanto_LoopData* data);
void tanto_FrameEnd(Tanto_LoopData*   data);

#endif /* end of include guard: UTILS_H */
