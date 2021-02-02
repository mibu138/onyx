#ifndef OBDN_I_INPUT_H
#define OBDN_I_INPUT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t  x;
    int16_t  y;
    uint8_t  buttonCode;
} Obdn_I_MouseData;

typedef struct {
    uint16_t width;
    uint16_t height;
} Obdn_I_ResizeData;

typedef union {
    uint32_t          keyCode;
    Obdn_I_MouseData  mouseData;
    Obdn_I_ResizeData resizeData;
} Obdn_I_EventData;

typedef enum {
    OBDN_I_KEYDOWN,
    OBDN_I_KEYUP,
    OBDN_I_MOUSEDOWN,
    OBDN_I_MOUSEUP,
    OBDN_I_MOTION,
    OBDN_I_RESIZE
} Obdn_I_EventType;

typedef struct Obdn_I_Event {
    Obdn_I_EventType type;
    Obdn_I_EventData data;
} Obdn_I_Event;

// returning true consumes the event
typedef bool (*Obdn_I_SubscriberFn)(const Obdn_I_Event*);

void obdn_i_Init(void);
void obdn_i_PublishEvent(Obdn_I_Event event);
void obdn_i_Subscribe(Obdn_I_SubscriberFn);
void obdn_i_ProcessEvents(void);
void obdn_i_CleanUp(void);

#endif /* end of include guard: OBDN_I_INPUT_H */
