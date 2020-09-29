/*
d_display.c
*/
#ifndef D_INIT_H
#define D_INIT_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "v_vulkan.h"

typedef struct {
    xcb_connection_t* connection;
    xcb_window_t      window;
} D_XcbWindow;

extern D_XcbWindow d_XcbWindow;

void d_Init(void);
void d_CleanUp(void);

#endif /* end of include guard: D_INIT_H */
