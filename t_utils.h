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

void bitprintRowLen(const void *const thing, const size_t bitcount, const size_t rowlength);
void bytePrint(const void* const thing, const size_t byteCount);
void bitprint(const void *const thing, const size_t bitcount);
uint64_t obdn_GetAligned(const uint64_t quantity, const uint32_t alignment);

typedef struct Obdn_Timer Obdn_Timer;

struct Obdn_Timer {
    struct timespec startTime;
    struct timespec endTime;
    clockid_t clockId;
};

void obdn_TimerStart(Obdn_Timer* t);
void obdn_TimerStop(Obdn_Timer* t);
void obdn_TimerInit(Obdn_Timer* t);
void obdn_PrintTime(const Obdn_Timer* t);

typedef struct Obdn_LoopStats Obdn_LoopStats;

struct Obdn_LoopStats {
    uint64_t frameCount;
    uint64_t nsTotal;
    unsigned long nsDelta;
    uint32_t shortestFrame;
    uint32_t longestFrame;
};

void obdn_LoopStatsInit(Obdn_LoopStats* stats);
void obdn_LoopStatsUpdate(const Obdn_Timer* t, Obdn_LoopStats* s);
void obdn_LoopSleep(const Obdn_LoopStats* s, const uint32_t nsTarget);

typedef struct {
    Obdn_Timer     timer;
    Obdn_LoopStats loopStats;
    const uint32_t  targetNs;
    const bool      printFps;
    const bool      printNs;
} Obdn_LoopData;

Obdn_LoopData obdn_CreateLoopData(const uint32_t targetNs, const bool printFps, const bool printNS);
void obdn_FrameStart(Obdn_LoopData* data);
void obdn_FrameEnd(Obdn_LoopData*   data);

#endif /* end of include guard: UTILS_H */
