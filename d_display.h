#ifndef OBDN_D_INIT_H
#define OBDN_D_INIT_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "v_vulkan.h"
#include <stdbool.h>

typedef struct {
    xcb_connection_t* connection;
    xcb_window_t      window;
} Obdn_D_XcbWindow;

// if null name will default to "floating"
Obdn_D_XcbWindow obdn_d_Init(const uint16_t width, const uint16_t height, const char* name);
void obdn_d_DrainEventQueue(void);
void obdn_d_CleanUp(void);

#endif /* end of include guard: D_INIT_H */
