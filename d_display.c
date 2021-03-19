#include "d_display.h"
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <string.h>
#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_util.h>
#include <X11/keysym.h>
#include "i_input.h"
#include "t_def.h"
#include <stdlib.h>

static Obdn_D_XcbWindow xcbWindow;

static xcb_key_symbols_t* pXcbKeySymbols;

static char windowName[32] = "floating";

uint32_t OBDN_WINDOW_WIDTH  = 1000;
uint32_t OBDN_WINDOW_HEIGHT = 1000;

static Obdn_I_EventData getKeyData(xcb_key_press_event_t* event)
{
    // XCB documentation is fucking horrible. fucking last parameter is called col. wtf? 
    // no clue what that means. ZERO documentation on this function. trash.
    xcb_keysym_t keySym = xcb_key_symbols_get_keysym(pXcbKeySymbols, event->detail, 0); 
    Obdn_I_EventData data;
    switch (keySym)
    {
        case XK_a:         data.keyCode = OBDN_KEY_A; break;
        case XK_b:         data.keyCode = OBDN_KEY_B; break;
        case XK_c:         data.keyCode = OBDN_KEY_C; break;
        case XK_d:         data.keyCode = OBDN_KEY_D; break;
        case XK_e:         data.keyCode = OBDN_KEY_E; break;
        case XK_f:         data.keyCode = OBDN_KEY_F; break;
        case XK_g:         data.keyCode = OBDN_KEY_G; break;
        case XK_h:         data.keyCode = OBDN_KEY_H; break;
        case XK_i:         data.keyCode = OBDN_KEY_I; break;
        case XK_j:         data.keyCode = OBDN_KEY_J; break;
        case XK_k:         data.keyCode = OBDN_KEY_K; break;
        case XK_l:         data.keyCode = OBDN_KEY_L; break;
        case XK_m:         data.keyCode = OBDN_KEY_M; break;
        case XK_n:         data.keyCode = OBDN_KEY_N; break;
        case XK_o:         data.keyCode = OBDN_KEY_O; break;
        case XK_p:         data.keyCode = OBDN_KEY_P; break;
        case XK_q:         data.keyCode = OBDN_KEY_Q; break;
        case XK_r:         data.keyCode = OBDN_KEY_R; break;
        case XK_s:         data.keyCode = OBDN_KEY_S; break;
        case XK_t:         data.keyCode = OBDN_KEY_T; break;
        case XK_u:         data.keyCode = OBDN_KEY_U; break;
        case XK_v:         data.keyCode = OBDN_KEY_V; break;
        case XK_w:         data.keyCode = OBDN_KEY_W; break;
        case XK_x:         data.keyCode = OBDN_KEY_X; break;
        case XK_y:         data.keyCode = OBDN_KEY_Y; break;
        case XK_z:         data.keyCode = OBDN_KEY_Z; break;
        case XK_1:         data.keyCode = OBDN_KEY_1; break;
        case XK_2:         data.keyCode = OBDN_KEY_2; break;
        case XK_3:         data.keyCode = OBDN_KEY_3; break;
        case XK_4:         data.keyCode = OBDN_KEY_4; break;
        case XK_5:         data.keyCode = OBDN_KEY_5; break;
        case XK_6:         data.keyCode = OBDN_KEY_6; break;
        case XK_7:         data.keyCode = OBDN_KEY_7; break;
        case XK_8:         data.keyCode = OBDN_KEY_8; break;
        case XK_9:         data.keyCode = OBDN_KEY_9; break;
        case XK_space:     data.keyCode = OBDN_KEY_SPACE; break;
        case XK_Control_L: data.keyCode = OBDN_KEY_CTRL; break;
        case XK_Escape:    data.keyCode = OBDN_KEY_ESC; break;
        default: data.keyCode = 0; break;
    }
    return data;
}

static Obdn_I_EventData getMouseData(const xcb_generic_event_t* event)
{
    xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)event;
    Obdn_I_EventData data;
    data.mouseData.x = motion->event_x;
    data.mouseData.y = motion->event_y;
    if (motion->detail == 1) data.mouseData.buttonCode = OBDN_MOUSE_LEFT; 
    else if (motion->detail == 2) data.mouseData.buttonCode = OBDN_MOUSE_MID; 
    else if (motion->detail == 3) data.mouseData.buttonCode = OBDN_MOUSE_RIGHT;
    return data;
}

static Obdn_I_EventData getResizeData(const xcb_generic_event_t* event)
{
    xcb_resize_request_event_t* resize = (xcb_resize_request_event_t*)event;
    Obdn_I_EventData data = {0};
    data.resizeData.height = resize->height;
    data.resizeData.width  = resize->width;
    return data;
}

static Obdn_I_EventData getConfigureData(const xcb_generic_event_t* event)
{
    xcb_configure_notify_event_t* resize = (xcb_configure_notify_event_t*)event;
    Obdn_I_EventData data = {0};
    data.resizeData.height = resize->height;
    data.resizeData.width  = resize->width;
    return data;
}

Obdn_D_XcbWindow obdn_d_Init(const uint16_t width, const uint16_t height, const char* name)
{
    if (name)
    {
        assert(strlen(name) < 32);
        strcpy(windowName, name);
    }
    OBDN_WINDOW_WIDTH  = width;
    OBDN_WINDOW_HEIGHT = height;
    int screenNum = 0;
    xcbWindow.connection =     xcb_connect(NULL, &screenNum);
    xcbWindow.window     =     xcb_generate_id(xcbWindow.connection);

    const xcb_setup_t* setup   = xcb_get_setup(xcbWindow.connection);
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
		XCB_EVENT_MASK_POINTER_MOTION |
    //  XCB_EVENT_MASK_RESIZE_REDIRECT |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
	//	XCB_EVENT_MASK_ENTER_WINDOW |
		XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_KEY_RELEASE |
	//	XCB_EVENT_MASK_LEAVE_WINDOW |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE;

    xcb_create_window(xcbWindow.connection, 
            XCB_COPY_FROM_PARENT,              // depth 
            xcbWindow.window,                  // window id
            screen->root,                      // parent
            0, 0,                              // x and y coordinate of new window
            OBDN_WINDOW_WIDTH, OBDN_WINDOW_HEIGHT, 
            0,                                 // border wdith 
            XCB_WINDOW_CLASS_COPY_FROM_PARENT, // class 
            XCB_COPY_FROM_PARENT,              // visual 
            mask, values);                          // masks (TODO: set to get inputs)

    xcb_change_property(xcbWindow.connection, 
            XCB_PROP_MODE_REPLACE, 
            xcbWindow.window, 
            XCB_ATOM_WM_NAME, 
            XCB_ATOM_STRING, 8, strlen(windowName), windowName);

    xcb_map_window(xcbWindow.connection, xcbWindow.window);
    xcb_flush(xcbWindow.connection);
    pXcbKeySymbols = xcb_key_symbols_alloc(xcbWindow.connection);
    printf("Obsidian Display initialized.\n");

    return xcbWindow;
}

void obdn_d_CleanUp(void)
{
    xcb_key_symbols_free(pXcbKeySymbols);
    xcb_flush(xcbWindow.connection);
    xcb_destroy_window(xcbWindow.connection, xcbWindow.window);
    xcb_disconnect(xcbWindow.connection);
    printf("Obsidian display cleaned up.\n");
}

void obdn_d_DrainEventQueue(void)
{
    xcb_generic_event_t* xEvent = NULL;
    while ((xEvent = xcb_poll_for_event(xcbWindow.connection)))
    {
        Obdn_I_Event event;
        switch (XCB_EVENT_RESPONSE_TYPE(xEvent))
        {
            case XCB_KEY_PRESS: 
                event.type = OBDN_I_KEYDOWN; 
                Obdn_I_EventData data = getKeyData((xcb_key_press_event_t*)xEvent);
                if (data.keyCode == 0) goto end;
                event.data = data;
                break;
            case XCB_KEY_RELEASE: 
                // bunch of extra stuff here dedicated to detecting autrepeats
                // the idea is that if a key-release event is detected, followed
                // by an immediate keypress of the same key, its an autorepeat.
                // its unclear to me whether very rapidly hitting a key could
                // result in the same thing, and wheter it is worthwhile 
                // accounting for that
                event.type = OBDN_I_KEYUP;
                data = getKeyData((xcb_key_press_event_t*)xEvent);
                if (data.keyCode == 0) goto end;
                event.data = data;
                // need to see if this is actually an auto repeat
                xcb_generic_event_t* next = xcb_poll_for_event(xcbWindow.connection);
                if (next) 
                {
                    Obdn_I_Event event2;
                    uint8_t type = XCB_EVENT_RESPONSE_TYPE(next);
                    event2.data = getKeyData((xcb_key_press_event_t*)next);
                    if (type == XCB_KEY_PRESS 
                            && event2.data.keyCode == event.data.keyCode)
                    {
                        // is likely an autorepeate
                        free(next);
                        goto end;
                    }
                    else
                    {
                        event2.type = OBDN_I_KEYUP;
                        obdn_i_PublishEvent(event);
                        event = event2;
                        free(next);
                    }
                    break;
                }
                break;
            case XCB_BUTTON_PRESS:
                event.type = OBDN_I_MOUSEDOWN;
                event.data = getMouseData(xEvent);
                break;
            case XCB_BUTTON_RELEASE:
                event.type = OBDN_I_MOUSEUP;
                event.data = getMouseData(xEvent);
                break;
            case XCB_MOTION_NOTIFY:
                event.type = OBDN_I_MOTION;
                event.data = getMouseData(xEvent);
                break;
            case XCB_RESIZE_REQUEST:
                event.type = OBDN_I_RESIZE;
                event.data = getResizeData(xEvent);
                break;
            case XCB_CONFIGURE_NOTIFY:
                event.type = OBDN_I_RESIZE;
                event.data = getConfigureData(xEvent);
                break;
            default: return;
        }
        obdn_i_PublishEvent(event);
end:
        free(xEvent);
    }
    // to do
}

