#ifndef TANTO_T_DEF_H
#define TANTO_T_DEF_H

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
#define TANTO_KEY_W     119
#define TANTO_KEY_A     97
#define TANTO_KEY_S     115
#define TANTO_KEY_D     100
#define TANTO_KEY_SPACE 32
// these values are arbitrary
#define TANTO_KEY_E     101
#define TANTO_KEY_Q     102
#define TANTO_KEY_P     103
#define TANTO_KEY_I     104
#define TANTO_KEY_C     105
#define TANTO_KEY_CTRL  24
#define TANTO_KEY_ESC   27
#define TANTO_KEY_R     140

#define TANTO_MOUSE_LEFT  1
#define TANTO_MOUSE_RIGHT 2
#define TANTO_MOUSE_MID   3

#define TANTO_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#endif /* end of include guard: DEF_H */
