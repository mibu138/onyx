#ifndef OBDN_V_IMAGE_H
#define OBDN_V_IMAGE_H

/*
 * Utility functions for Obdn Images
 */

#include "memory.h"
#include "command.h"


typedef enum {
    OBDN_V_IMAGE_FILE_TYPE_PNG,
    OBDN_V_IMAGE_FILE_TYPE_JPG
} Obdn_V_ImageFileType;

Obdn_Image obdn_CreateImageAndSampler(
    Obdn_Memory* memory,
    const uint32_t width, 
    const uint32_t height,
    const VkFormat format,
    VkImageUsageFlags extraUsageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const uint32_t mipLevels,
    const VkFilter filter,
    const Obdn_MemoryType memType);

void obdn_TransitionImageLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout, Obdn_Image* image);

void obdn_CopyBufferToImage(const Obdn_BufferRegion* region,
        Obdn_Image* image);

void obdn_CmdCopyImageToBuffer(VkCommandBuffer cmdbuf, uint32_t miplevel,
        const Obdn_Image* image, Obdn_BufferRegion* region);

void obdn_CmdCopyBufferToImage(const VkCommandBuffer cmdbuf, uint32_t mipLevel, const Obdn_BufferRegion* region,
        Obdn_Image* image);

void obdn_CmdTransitionImageLayout(const VkCommandBuffer cmdbuf, const Obdn_Barrier barrier, 
        const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint32_t mipLevels, VkImage image);

// Does not check for existence of file at filepath.
// all images created on the graphic queue. for now.
// extraUsageFlags are additional usage flags that will be set on 
// top of the ones that are needed such as TRANSFER_DST and 
// TRANSFER_SRC if mip maps are needed. No harm in over specifying.
void obdn_LoadImage(Obdn_Memory* memory, const char* filepath, const uint8_t channelCount, const VkFormat format,
    VkImageUsageFlags extraUsageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const VkFilter filter,
    const VkImageLayout layout,
    const bool createMips,
    Obdn_MemoryType memoryType,
    Obdn_Image* image);

void obdn_SaveImage(Obdn_Memory* memory, Obdn_Image* image, Obdn_V_ImageFileType fileType, const char* filename);

void obdn_v_ClearColorImage(Obdn_Image* image);

#endif /* end of include guard: OBDN_V_IMAGE_H */
