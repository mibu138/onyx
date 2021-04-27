#include "common.h"
#include <hell/common.h>
#include <hell/cmd.h>
#include <stdarg.h>
#include <stdio.h>
#include <obsidian/v_video.h>

#define MAX_PRINT_MSG 256
#define HEADER "OBSIDIAN: "

#define STR_100_MB "104857600"
#define STR_256_MB "268435456"

void obdn_Announce(const char* fmt, ...)
{
    va_list argptr;
    char    msg[MAX_PRINT_MSG];
    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);
    hell_Print("%s%s", HEADER, msg);
}

void obdn_Init()
{
    Obdn_V_Config cfg;
    cfg.memorySizes.hostGraphicsBufferMemorySize   = hell_c_GetVar("hstGraphicsBufferMemorySize", STR_100_MB, HELL_C_VAR_ARCHIVE_BIT)->value;
    cfg.memorySizes.deviceGraphicsBufferMemorySize = hell_c_GetVar("devGraphicsBufferMemorySize", STR_100_MB, HELL_C_VAR_ARCHIVE_BIT)->value;
    cfg.memorySizes.deviceGraphicsImageMemorySize  =  hell_c_GetVar("devGraphicsImageSize", STR_256_MB, HELL_C_VAR_ARCHIVE_BIT)->value;
#ifdef NDEBUG
    cfg.validationEnabled = false;
#else
    cfg.validationEnabled = true;
#endif
}
