#ifndef OBDN_V_DEF_H
#define OBDN_V_DEF_H

#include "v_vulkan.h"
#include <stdbool.h>

#define OBDN_FRAME_COUNT 2

typedef struct {
    VkDeviceSize hostGraphicsBufferMemorySize; 
    VkDeviceSize deviceGraphicsBufferMemorySize;
    VkDeviceSize deviceGraphicsImageMemorySize;
    VkDeviceSize hostTransferBufferMemorySize;
    VkDeviceSize deviceExternalGraphicsImageMemorySize;
} Obdn_V_MemorySizes;

typedef struct {
    bool               rayTraceEnabled;
    bool               validationEnabled;
    Obdn_V_MemorySizes memorySizes;
} Obdn_V_Config;

#endif /* end of include guard: V_DEF_H */

