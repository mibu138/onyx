#include "common.h"
#include <hell/common.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_PRINT_MSG 256
#define HEADER "OBSIDIAN: "

void obdn_Announce(const char* fmt, ...)
{
    va_list argptr;
    char    msg[MAX_PRINT_MSG];
    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);
    hell_Print("%s%s", HEADER, msg);
}

