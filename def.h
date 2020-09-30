#ifndef DEF_H
#define DEF_H

#include <stdio.h>
#include <stdbool.h>
#include <setjmp.h>

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE > 0
#define V1_PRINT(msg, args...) printf(msg, ## args)
#else
#define V1_PRINT(msg, args...)
#endif

// key values are ascii lower case
#define KEY_W     119
#define KEY_A     97
#define KEY_S     115
#define KEY_D     100
#define KEY_SPACE 32
// these values are arbitrary
#define KEY_CTRL  24
#define KEY_ESC   27
#define KEY_R     140

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

extern jmp_buf exit_game;

typedef enum {
    MODE_RASTER,
    MODE_RAY,
} ModeID;

typedef struct {
    ModeID mode;
    bool   renderNeedsUpdate;
    bool   shouldRun;
} Parms;

extern Parms parms; //defined in g_game.c

#endif /* end of include guard: DEF_H */
