#include "d_display.h"
#include "def.h"
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <string.h>

D_XcbWindow d_XcbWindow;

static const char* windowName = "floating";

void d_Init(void)
{
    int screenNum = 0;
    d_XcbWindow.connection =     xcb_connect(NULL, &screenNum);
    d_XcbWindow.window     =     xcb_generate_id(d_XcbWindow.connection);

    const xcb_setup_t* setup = xcb_get_setup(d_XcbWindow.connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

    for (int i = 0; i < screenNum; i++)
    {
        xcb_screen_next(&iter);   
    }

    xcb_screen_t* screen = iter.data;

    uint32_t values[2];
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	values[0] = screen->black_pixel;
//	we need to limit what events we are interested in.
//	otherwise the queue will fill up with garbage
	values[1] = //XCB_EVENT_MASK_EXPOSURE |
//		XCB_EVENT_MASK_POINTER_MOTION |
//		XCB_EVENT_MASK_ENTER_WINDOW |
		XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_KEY_RELEASE;
//		XCB_EVENT_MASK_LEAVE_WINDOW |
//		XCB_EVENT_MASK_BUTTON_PRESS |
//		XCB_EVENT_MASK_BUTTON_RELEASE;
//

    xcb_create_window(d_XcbWindow.connection, 
            XCB_COPY_FROM_PARENT,              // depth 
            d_XcbWindow.window,                  // window id
            screen->root,                      // parent
            0, 0,                              // x and y coordinate of new window
            WINDOW_WIDTH, WINDOW_HEIGHT, 
            0,                                 // border wdith 
            XCB_WINDOW_CLASS_COPY_FROM_PARENT, // class 
            XCB_COPY_FROM_PARENT,              // visual 
            mask, values);                          // masks (TODO: set to get inputs)

    xcb_change_property(d_XcbWindow.connection, 
            XCB_PROP_MODE_REPLACE, 
            d_XcbWindow.window, 
            XCB_ATOM_WM_NAME, 
            XCB_ATOM_STRING, 8, strlen(windowName), windowName);

    xcb_map_window(d_XcbWindow.connection, d_XcbWindow.window);
    //xcb_flush(d_XcbWindow.connection);
}

void d_CleanUp(void)
{
    xcb_disconnect(d_XcbWindow.connection);
}
