#ifndef TANTO_T_DEF_H
#define TANTO_T_DEF_H

#include <stdio.h>
#include <stdbool.h>
#include <setjmp.h>
#include <carbon/carbon.h>

#ifndef NDEBUG
#ifndef VERBOSE
#define VERBOSE 1
#endif
// print if condition is true
// make sure arguments have NO side effects
#define TANTO_DEBUG_PRINT(msg, args...) printf("%s: "msg"\n", __PRETTY_FUNCTION__, ## args)
#define TANTO_COND_PRINT(condition, msg, args...) if (condition) printf("%s: "msg"\n", __PRETTY_FUNCTION__, ## args)
#else
#define TANTO_COND_PRINT(condition, msg, args...) ()
#define TANTO_DEBUG_PRINT(msg, args...) ()
#endif

#if VERBOSE > 0
#define V1_PRINT(msg, args...) printf(msg, ## args)
#else
#define V1_PRINT(msg, args...)
#endif

// key values are ascii lower case
#define TANTO_KEY_SPACE 32
#define TANTO_KEY_CTRL  24
#define TANTO_KEY_ESC   27

#define TANTO_KEY_A    97
#define TANTO_KEY_B    98
#define TANTO_KEY_C    99
#define TANTO_KEY_D    100
#define TANTO_KEY_E    101
#define TANTO_KEY_F    102
#define TANTO_KEY_G    103
#define TANTO_KEY_H    104
#define TANTO_KEY_I    105
#define TANTO_KEY_J    106
#define TANTO_KEY_K    107
#define TANTO_KEY_L    108
#define TANTO_KEY_M    109
#define TANTO_KEY_N    110
#define TANTO_KEY_O    111
#define TANTO_KEY_P    112
#define TANTO_KEY_Q    113
#define TANTO_KEY_R    114
#define TANTO_KEY_S    115
#define TANTO_KEY_T    116
#define TANTO_KEY_U    117
#define TANTO_KEY_V    118
#define TANTO_KEY_W    119
#define TANTO_KEY_X    120
#define TANTO_KEY_Y    121
#define TANTO_KEY_Z    122

#define TANTO_MOUSE_LEFT  1
#define TANTO_MOUSE_RIGHT 2
#define TANTO_MOUSE_MID   3

#define TANTO_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#endif /* end of include guard: DEF_H */
