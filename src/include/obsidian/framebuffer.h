#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include "memory.h"

#define OBDN_MAX_AOVS 8

typedef struct Obdn_Framebuffer {
    Obdn_Image aovs[OBDN_MAX_AOVS];
    uint32_t   width;
    uint32_t   height;
    bool       dirty;
    uint8_t    index;
    uint8_t    aovCount;
} Obdn_Framebuffer;

typedef struct Obdn_AovInfo {
    VkFormat           format;
    VkImageUsageFlags  usageFlags;
    VkImageAspectFlags aspectFlags;
} Obdn_AovInfo;

#endif /* end of include guard: FRAMEBUFFER_H */
