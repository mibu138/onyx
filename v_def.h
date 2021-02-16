#ifndef OBDN_V_DEF_H
#define OBDN_V_DEF_H

#include "v_vulkan.h"
#include <stdbool.h>
#include <vulkan/vulkan_core.h>

#define OBDN_FRAME_COUNT 2

typedef struct {
    bool rayTraceEnabled;
    bool validationEnabled;
} Obdn_V_Config;

typedef struct {
    VkDeviceSize hostGraphicsBufferMemorySize; 
    VkDeviceSize deviceGraphicsBufferMemorySize;
    VkDeviceSize deviceGraphicsImageMemorySize;
    VkDeviceSize hostTransferBufferMemorySize;
    VkDeviceSize deviceExternalGraphicsImageMemorySize;
} Obdn_V_MemorySizes;

extern Obdn_V_Config obdn_v_config;

#endif /* end of include guard: V_DEF_H */

