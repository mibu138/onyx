#ifndef TANTO_V_COMMAND_H
#define TANTO_V_COMMAND_H

#include "v_def.h"

typedef struct {
    VkCommandPool   handle;
    VkCommandBuffer buffer;
    uint32_t        queueFamily;
} Tanto_V_CommandPool;

Tanto_V_CommandPool tanto_v_RequestOneTimeUseCommand(void);

void tanto_v_SubmitOneTimeCommandAndWait(Tanto_V_CommandPool* pool, const uint32_t queueIndex);

#endif /* end of include guard: TANTO_V_COMMAND_H */
