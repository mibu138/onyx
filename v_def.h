#ifndef TANTO_V_DEF_H
#define TANTO_V_DEF_H

#include "v_vulkan.h"
#include <stdbool.h>

#define TANTO_FRAME_COUNT 2

typedef struct {
    bool rayTraceEnabled;
    bool validationEnabled;
} Tanto_V_Config;

extern Tanto_V_Config tanto_v_config;

#endif /* end of include guard: V_DEF_H */

