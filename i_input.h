#ifndef I_INPUT_H
#define I_INPUT_H

#include <stdint.h>

typedef uint32_t I_EventData;

typedef enum {
    i_Keydown,
    i_Keyup,
} I_EventType;

typedef struct i_event {
    I_EventType type;
    I_EventData data;
} I_Event;

void i_Init(void);
void i_GetEvents(void);
void i_ProcessEvents(void);
void i_CleanUp(void);

#endif /* end of include guard: I_INPUT_H */
