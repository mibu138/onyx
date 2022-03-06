#ifndef ONYX_FRAME_H
#define ONYX_FRAME_H

#include <stdint.h>
#include "memory.h"

#define ONYX_MAX_AOVS 8

typedef struct Onyx_Frame {
    Onyx_Image aovs[ONYX_MAX_AOVS];
    uint32_t   width;
    uint32_t   height;
    bool       dirty;
    uint8_t    index;
    uint8_t    aovCount;
} Onyx_Frame;

typedef struct Onyx_AovInfo {
    VkFormat           format;
    VkImageUsageFlags  usageFlags;
    VkImageAspectFlags aspectFlags;
} Onyx_AovInfo;

void onyx_CreateFrames(Onyx_Memory* memory, uint8_t count, uint8_t aovCount, const Onyx_AovInfo* aov_infos, 
        uint32_t width, uint32_t height, Onyx_Frame* frames);

#endif /* end of include guard: ONYX_FRAME_H */
