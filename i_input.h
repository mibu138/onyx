#ifndef TANTO_I_INPUT_H
#define TANTO_I_INPUT_H

#include <stdint.h>

typedef struct {
    int16_t  x;
    int16_t  y;
    uint8_t  buttonCode;
} Tanto_I_MouseData;

typedef union {
    uint32_t          keyCode;
    Tanto_I_MouseData mouseData;
} Tanto_I_EventData;

typedef enum {
    TANTO_I_KEYDOWN,
    TANTO_I_KEYUP,
    TANTO_I_MOUSEDOWN,
    TANTO_I_MOUSEUP,
    TANTO_I_MOTION
} Tanto_I_EventType;

typedef struct Tanto_I_Event {
    Tanto_I_EventType type;
    Tanto_I_EventData data;
} Tanto_I_Event;

typedef void (*Tanto_I_SubscriberFn)(const Tanto_I_Event*);

void tanto_i_Init(void);
void tanto_i_GetEvents(void);
void tanto_i_Subscribe(Tanto_I_SubscriberFn);
void tanto_i_ProcessEvents(void);
void tanto_i_CleanUp(void);

#endif /* end of include guard: TANTO_I_INPUT_H */
