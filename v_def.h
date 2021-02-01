#ifndef OBDN_V_DEF_H
#define OBDN_V_DEF_H

#include "v_vulkan.h"
#include <stdbool.h>

#define OBDN_FRAME_COUNT 2

typedef struct {
    bool rayTraceEnabled;
    bool validationEnabled;
} Obdn_V_Config;

extern Obdn_V_Config obdn_v_config;

#endif /* end of include guard: V_DEF_H */

