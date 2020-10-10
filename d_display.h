#ifndef TANTO_D_INIT_H
#define TANTO_D_INIT_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include "v_vulkan.h"

typedef struct {
    xcb_connection_t* connection;
    xcb_window_t      window;
} Tanto_D_XcbWindow;

extern Tanto_D_XcbWindow d_XcbWindow;

void tanto_d_Init(void);
void tanto_d_CleanUp(void);

#endif /* end of include guard: D_INIT_H */
