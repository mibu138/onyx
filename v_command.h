#ifndef TANTO_V_COMMAND_H
#define TANTO_V_COMMAND_H

#include "v_def.h"

typedef struct {
    VkCommandPool   handle;
    VkCommandBuffer buffer;
} Tanto_V_CommandPool;

Tanto_V_CommandPool tanto_v_RequestOneTimeUseCommand(const uint32_t queueIndex);

#endif /* end of include guard: TANTO_V_COMMAND_H */
