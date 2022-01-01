#ifndef OBDN_FRAME_H
#define OBDN_FRAME_H

#include <stdint.h>
#include "memory.h"

#define OBDN_MAX_AOVS 8

typedef struct Obdn_Frame {
    Obdn_Image aovs[OBDN_MAX_AOVS];
    uint32_t   width;
    uint32_t   height;
    bool       dirty;
    uint8_t    index;
    uint8_t    aovCount;
} Obdn_Frame;

typedef struct Obdn_AovInfo {
    VkFormat           format;
    VkImageUsageFlags  usageFlags;
    VkImageAspectFlags aspectFlags;
} Obdn_AovInfo;

void obdn_CreateFrames(Obdn_Memory* memory, uint8_t count, uint8_t aovCount, const Obdn_AovInfo* aov_infos, 
        uint32_t width, uint32_t height, Obdn_Frame* frames);

#endif /* end of include guard: OBDN_FRAME_H */
