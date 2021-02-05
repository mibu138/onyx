#ifndef OBDN_V_IMAGE_H
#define OBDN_V_IMAGE_H

/*
 * Utility functions for Obdn Images
 */

#include "v_memory.h"
#include "v_command.h"

typedef enum {
    OBDN_V_IMAGE_FILE_TYPE_PNG,
    OBDN_V_IMAGE_FILE_TYPE_JPG
} Obdn_V_ImageFileType;

Obdn_V_Image obdn_v_CreateImageAndSampler(
    const uint32_t width, 
    const uint32_t height,
    const VkFormat format,
    const VkImageUsageFlags usageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const uint32_t mipLevels,
    const VkFilter filter,
    const Obdn_V_MemoryType memType);

void obdn_v_TransitionImageLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout, Obdn_V_Image* image);

void obdn_v_CopyBufferToImage(const Obdn_V_BufferRegion* region,
        Obdn_V_Image* image);

void obdn_v_CmdCopyBufferToImage(const VkCommandBuffer cmdbuf, const Obdn_V_BufferRegion* region,
        Obdn_V_Image* image);

void obdn_v_CmdCopyImageToBuffer(const VkCommandBuffer cmdbuf, const Obdn_V_Image* image, const VkImageAspectFlags aspectMask, Obdn_V_BufferRegion* region);
void obdn_v_CmdTransitionImageLayout(const VkCommandBuffer cmdbuf, const Obdn_V_Barrier barrier, 
        const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint32_t mipLevels, VkImage image);

void obdn_v_LoadImage(const char* filename, const uint8_t channelCount, const VkFormat format,
    const VkImageUsageFlags usageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const VkFilter filter,
    const uint32_t queueFamilyIndex, 
    const VkImageLayout, 
    const bool createMipMaps,
    Obdn_V_Image* image);

void obdn_v_SaveImage(Obdn_V_Image* image, Obdn_V_ImageFileType fileType, const char* name);

void obdn_v_ClearColorImage(Obdn_V_Image* image);

#endif /* end of include guard: OBDN_V_IMAGE_H */
