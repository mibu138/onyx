#include "i_input.h"
#include "d_display.h"
#include "t_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 32
#define MAX_SUBSCRIBERS 5

static Obdn_I_Event events[MAX_EVENTS];
static int     eventHead;
static int     eventTail;

static Obdn_I_SubscriberFn subscribers[MAX_SUBSCRIBERS];
static int subscriberCount;

void obdn_i_Init(void)
{
    subscriberCount = 0;
    eventHead = 0;
    eventTail = 0;
    printf("Obsidian Input initialized.\n");
}

void obdn_i_PublishEvent(Obdn_I_Event event)
{
    events[eventHead] = event;
    eventHead = (eventHead + 1) % MAX_EVENTS;
}

void obdn_i_Subscribe(const Obdn_I_SubscriberFn fn)
{
    assert(subscriberCount < MAX_SUBSCRIBERS);
    subscribers[subscriberCount++] = fn;
}

void obdn_i_Unsubscribe(const Obdn_I_SubscriberFn fn)
{
    assert(subscriberCount > 0);
    printf("I) Unsubscribing fn...\n");
    int fnIndex = -1;
    for (int i = 0; i < subscriberCount; i++)
    {
        if (subscribers[i] == fn)
        {
            fnIndex = i;
            break;
        }
    }
    if (fnIndex != -1)
        memmove(subscribers + fnIndex, 
                subscribers + fnIndex + 1, 
                (--subscriberCount - fnIndex) * sizeof(*subscribers)); // should only decrement the count if fnIndex is 0
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
}
