#ifndef OBDN_T_DEF_H
#define OBDN_T_DEF_H

#include <stdio.h>
#include <stdbool.h>
#include <setjmp.h>
#include <coal/coal.h>
#include <stdint.h>

#ifndef NDEBUG
#ifndef VERBOSE
#define VERBOSE 1
#endif
// print if condition is true
// make sure arguments have NO side effects
#define OBDN_DEBUG_PRINT(msg, args...) printf("%s: " msg "\n", __PRETTY_FUNCTION__, ## args)
#define OBDN_COND_PRINT(condition, msg, args...) if (condition) printf("%s: " msg "\n", __PRETTY_FUNCTION__, ## args)
#else
#define OBDN_COND_PRINT(condition, msg, args...) 
#define OBDN_DEBUG_PRINT(msg, args...) 
#endif

#if VERBOSE > 0
#define V1_PRINT(msg, args...) printf(msg, ## args)
#else
#define V1_PRINT(msg, args...)
#endif

// key values are ascii lower case
#define OBDN_KEY_SPACE 32
#define OBDN_KEY_CTRL  24
#define OBDN_KEY_ESC   27

#define OBDN_KEY_A    97
#define OBDN_KEY_B    98
#define OBDN_KEY_C    99
#define OBDN_KEY_D    100
#define OBDN_KEY_E    101
#define OBDN_KEY_F    102
#define OBDN_KEY_G    103
#define OBDN_KEY_H    104
#define OBDN_KEY_I    105
#define OBDN_KEY_J    106
#define OBDN_KEY_K    107
#define OBDN_KEY_L    108
#define OBDN_KEY_M    109
#define OBDN_KEY_N    110
#define OBDN_KEY_O    111
#define OBDN_KEY_P    112
#define OBDN_KEY_Q    113
#define OBDN_KEY_R    114
#define OBDN_KEY_S    115
#define OBDN_KEY_T    116
#define OBDN_KEY_U    117
#define OBDN_KEY_V    118
#define OBDN_KEY_W    119
#define OBDN_KEY_X    120
#define OBDN_KEY_Y    121
#define OBDN_KEY_Z    122
#define OBDN_KEY_1    49
#define OBDN_KEY_2    50
#define OBDN_KEY_3    51
#define OBDN_KEY_4    52
#define OBDN_KEY_5    53
#define OBDN_KEY_6    54
#define OBDN_KEY_7    55
#define OBDN_KEY_8    56
#define OBDN_KEY_9    57

#define OBDN_MOUSE_LEFT  1
#define OBDN_MOUSE_RIGHT 2
#define OBDN_MOUSE_MID   3

#define OBDN_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

typedef uint32_t Obdn_Mask;

#endif /* end of include guard: DEF_H */
