#ifndef ONYX_V_IMAGE_H
#define ONYX_V_IMAGE_H

/*
 * Utility functions for Onyx Images
 */

#include "memory.h"
#include "command.h"


typedef enum {
    ONYX_V_IMAGE_FILE_TYPE_PNG,
    ONYX_V_IMAGE_FILE_TYPE_JPG
} Onyx_V_ImageFileType;

Onyx_Image onyx_CreateImageAndSampler(
    Onyx_Memory* memory,
    const uint32_t width, 
    const uint32_t height,
    const VkFormat format,
    VkImageUsageFlags extraUsageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const uint32_t mipLevels,
    const VkFilter filter,
    const Onyx_MemoryType memType);

void onyx_TransitionImageLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout, Onyx_Image* image);

void onyx_CopyBufferToImage(const Onyx_BufferRegion* region,
        Onyx_Image* image);

void onyx_CmdCopyImageToBuffer(VkCommandBuffer cmdbuf, uint32_t miplevel,
        const Onyx_Image* image, Onyx_BufferRegion* region);

VkImageBlit onyx_ImageBlitSimpleColor(uint32_t srcWidth, uint32_t srcHeight, uint32_t dstWidth, uint32_t dstHeight, 
        uint32_t srcMipLevel, uint32_t dstMipLevel);

void onyx_CmdCopyBufferToImage(const VkCommandBuffer cmdbuf, uint32_t mipLevel, const Onyx_BufferRegion* region,
        Onyx_Image* image);

void onyx_CmdTransitionImageLayout(const VkCommandBuffer cmdbuf, const Onyx_Barrier barrier, 
        const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint32_t mipLevels, VkImage image);

// Does not check for existence of file at filepath.
// all images created on the graphic queue. for now.
// extraUsageFlags are additional usage flags that will be set on 
// top of the ones that are needed such as TRANSFER_DST and 
// TRANSFER_SRC if mip maps are needed. No harm in over specifying.
void onyx_LoadImage(Onyx_Memory* memory, const char* filepath, const uint8_t channelCount, const VkFormat format,
    VkImageUsageFlags extraUsageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const VkFilter filter,
    const VkImageLayout layout,
    const bool createMips,
    Onyx_MemoryType memoryType,
    Onyx_Image* image);

void onyx_LoadImageData(Onyx_Memory* memory, int w, int h, uint8_t channelCount, void* data, const VkFormat format,
    VkImageUsageFlags usageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const VkFilter filter,
    const VkImageLayout layout,
    const bool createMips,
    Onyx_MemoryType memoryType,
    Onyx_Image* image);

int
onyx_write_image_to_png_buf(Onyx_Image* img,
                            VkImageLayout layout,
                            uint8_t** png_buf,
                            int* png_buf_size);

int 
onyx_copy_png_buf_to_image(const uint8_t* png_buf,
                            const int png_buf_size,
                            Onyx_Memory* memory,
                            VkImageLayout layout,
                            Onyx_Image* img);

int onyx_copy_image_to_buffer(Onyx_Image* image,
                              VkImageLayout orig_layout,
                              Onyx_BufferRegion* region);


void onyx_SaveImage(Onyx_Memory* memory, Onyx_Image* image, Onyx_V_ImageFileType fileType, VkImageLayout image_layout, 
        const char* filename);

void onyx_v_ClearColorImage(Onyx_Image* image);

VkImageSubresourceRange onyx_GetImageSubresourceRange(const Onyx_Image* img);

VkImageCopy
onyx_ImageCopySimpleColor(uint32_t width, uint32_t height, uint32_t srcOffsetX,
                          uint32_t srcOffsetY, uint32_t srcMipLevel,
                          uint32_t dstOffsetX, uint32_t dstOffsetY,
                          uint32_t dstMipLevel);

uint32_t onyx_CalcMipLevelsForImage(uint32_t width, uint32_t height);


#endif /* end of include guard: ONYX_V_IMAGE_H */
