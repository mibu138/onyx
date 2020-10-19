#ifndef TANTO_V_IMAGE_H
#define TANTO_V_IMAGE_H

/*
 * Utility functions for Tanto Images
 */

#include "v_memory.h"

typedef enum {
    TANTO_V_IMAGE_FILE_PNG_TYPE
} Tanto_V_ImageFileType;

Tanto_V_Image tanto_v_CreateImageAndSampler(
    const uint32_t width, 
    const uint32_t height,
    const VkFormat format,
    const VkImageUsageFlags usageFlags,
    const VkImageAspectFlags aspectMask,
    const VkFilter filter);

void tanto_v_TransitionImageLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout, Tanto_V_Image* image);

void tanto_v_SaveImage(Tanto_V_Image* image, Tanto_V_ImageFileType fileType);

#endif /* end of include guard: TANTO_V_IMAGE_H */
