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

#define OBDN_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

typedef uint32_t Obdn_Mask;

#endif /* end of include guard: DEF_H */
