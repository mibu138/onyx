#include "i_input.h"
#include "d_display.h"
#include "t_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_util.h>
#include <X11/keysym.h>

#define MAX_EVENTS 32
#define MAX_SUBSCRIBERS 5

static xcb_key_symbols_t* pXcbKeySymbols;

static Obdn_I_Event events[MAX_EVENTS];
static int     eventHead;
static int     eventTail;

static Obdn_I_SubscriberFn subscribers[MAX_SUBSCRIBERS];
static int subscriberCount;

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

static void postEvent(Obdn_I_Event event)
{
    events[eventHead] = event;
    eventHead = (eventHead + 1) % MAX_EVENTS;
}

void obdn_i_Init(void)
{
    subscriberCount = 0;
    eventHead = 0;
    eventTail = 0;
    pXcbKeySymbols = xcb_key_symbols_alloc(d_XcbWindow.connection);
    printf("Obdn Input initialized.\n");
}

void obdn_i_GetEvents(void)
{
    xcb_generic_event_t* xEvent = NULL;
    while ((xEvent = xcb_poll_for_event(d_XcbWindow.connection)))
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
                xcb_generic_event_t* next = xcb_poll_for_event(d_XcbWindow.connection);
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
                        postEvent(event);
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
        postEvent(event);
end:
        free(xEvent);
    }
    // to do
}

void obdn_i_Subscribe(Obdn_I_SubscriberFn fn)
{
    assert(subscriberCount < MAX_SUBSCRIBERS);
    subscribers[subscriberCount++] = fn;
}

void obdn_i_ProcessEvents(void)
{
    Obdn_I_Event* event;
    for ( ; eventTail != eventHead; eventTail = (eventTail + 1) % MAX_EVENTS) 
    {
        //printf("e");
        event = &events[eventTail];   
        for (int i = 0; i < subscriberCount; i++) 
        {
            if ( subscribers[i](event) ) break;
        }
    }
    //printf("\n");
}

void obdn_i_CleanUp(void)
{
    xcb_key_symbols_free(pXcbKeySymbols);
}
