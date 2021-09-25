#include "image.h"
#include "memory.h"
#include "video.h"
#include "common.h"
#include "command.h"
#include "dtags.h"
#include <hell/debug.h>
#include <string.h>
#include "private.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#define IMG_OUT_DIR "/out/images/"

typedef Obdn_Command      Command;
typedef Obdn_Barrier      Barrier;
typedef Obdn_BufferRegion BufferRegion;
typedef Obdn_Image        Image;

#define DPRINT(fmt, ...) hell_DebugPrint(OBDN_DEBUG_TAG_IMG, fmt, ##__VA_ARGS__)

static void createMipMaps(const Obdn_Instance* intstance, const VkFilter filter, const VkImageLayout finalLayout, Image* image)
{
    DPRINT("Creating mips for image %p\n", image->handle);

    Command cmd = obdn_CreateCommand(intstance, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_BeginCommandBuffer(cmd.buffer);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = image->handle,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .subresourceRange.levelCount = 1,
    };

    const uint32_t mipLevels = image->mipLevels;

    uint32_t mipWidth  = image->extent.width;
    uint32_t mipHeight = image->extent.height;

    for (int i = 1; i < mipLevels; i++) 
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0, 0, NULL, 0, NULL, 1, &barrier);

        const VkImageBlit blit = {
            .srcOffsets = {
                {0, 0, 0},
                {mipWidth, mipHeight, 1}},
            .dstOffsets = {
                {0, 0, 0},
                {mipWidth  > 1 ? mipWidth  / 2 : 1,
                 mipHeight > 1 ? mipHeight / 2 : 1, 
                 1}},
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1},
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1}
        };

        vkCmdBlitImage(cmd.buffer, image->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                1, &blit, filter);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = finalLayout,
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cmd.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
                0, 0, NULL, 0, NULL, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // last one
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = finalLayout;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;

    vkCmdPipelineBarrier(cmd.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
            0, 0, NULL, 0, NULL, 1, &barrier);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitAndWait(&cmd, 0);

    obdn_DestroyCommand(cmd);

    image->layout = finalLayout;
}

Obdn_Image obdn_CreateImageAndSampler(
    Obdn_Memory* memory,
    const uint32_t width, 
    const uint32_t height,
    const VkFormat format,
    const VkImageUsageFlags usageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const uint32_t mipLevels,
    const VkFilter filter,
    const Obdn_MemoryType memType)
{
    Obdn_Image image = obdn_CreateImage(memory, width, height, format, usageFlags, aspectMask, sampleCount, mipLevels, memType);

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0, //TODO understand the actual lod calculation. -0.5 just seems to look better to me
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 1.0,
        .compareEnable = VK_FALSE,
        .minLod = 0.0,
        .maxLod = mipLevels, // must both be 0 when using unnormalizedCoordinates
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE // allow us to window coordinates in frag shader
    };

    V_ASSERT( vkCreateSampler(memory->instance->device, &samplerInfo, NULL, &image.sampler) );

    return image;
}

void obdn_CmdTransitionImageLayout(const VkCommandBuffer cmdbuf, const Barrier barrier, 
        const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint32_t mipLevels, VkImage image)
{
    VkImageSubresourceRange subResRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .layerCount = 1,
    };

    VkImageMemoryBarrier imgBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = image,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .subresourceRange = subResRange,
        .srcAccessMask = barrier.srcAccessMask,
        .dstAccessMask = barrier.dstAccessMask,
    };

    vkCmdPipelineBarrier(cmdbuf, 
            barrier.srcStageFlags, 
            barrier.dstStageFlags, 0, 0, NULL, 0, NULL, 1, &imgBarrier);
}

void obdn_CmdCopyBufferToImage(VkCommandBuffer cmdbuf, uint32_t mipLevel, const Obdn_BufferRegion* region,
        Obdn_Image* image)
{
    const VkImageSubresourceLayers subRes = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .layerCount = 1, 
        .mipLevel = mipLevel,
    };

    const VkOffset3D imgOffset = {
        .x = 0,
        .y = 0,
        .z = 0
    };

    const VkBufferImageCopy imgCopy = {
        .imageOffset = imgOffset,
        .imageExtent = image->extent,
        .imageSubresource = subRes,
        .bufferOffset = region->offset,
        .bufferImageHeight = 0,
        .bufferRowLength = 0
    };

    DPRINT("Copying buffer to image...\n");
    vkCmdCopyBufferToImage(cmdbuf, region->buffer, image->handle, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopy);
}

void obdn_CmdCopyImageToBuffer(VkCommandBuffer cmdbuf, uint32_t miplevel,
        const Obdn_Image* image, Obdn_BufferRegion* region)
{
    assert( miplevel < image->mipLevels);
    const VkImageSubresourceLayers subRes = {.aspectMask     = image->aspectMask,
                                             .baseArrayLayer = 0,
                                             .layerCount     = 1,
                                             .mipLevel       = miplevel};

    const VkBufferImageCopy imgCopy = {.imageOffset       = {0, 0, 0},
                                       .imageExtent       = image->extent,
                                       .imageSubresource  = subRes,
                                       .bufferOffset      = region->offset,
                                       .bufferImageHeight = 0,
                                       .bufferRowLength   = 0};

    vkCmdCopyImageToBuffer(cmdbuf, image->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           region->buffer, 1, &imgCopy);
}

void obdn_TransitionImageLayout(const VkImageLayout oldLayout, const VkImageLayout newLayout, Obdn_Image* image)
{
    Command cmd = obdn_CreateCommand(image->pChain->memory->instance, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_BeginCommandBuffer(cmd.buffer);

    Barrier barrier = {
        .srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .srcAccessMask = 0,
        .dstStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = 0, 
    };

    obdn_CmdTransitionImageLayout(cmd.buffer, barrier, oldLayout, newLayout, image->mipLevels, image->handle);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitAndWait(&cmd, 0);

    obdn_DestroyCommand(cmd);

    image->layout = newLayout;
}

void obdn_CopyBufferToImage(const Obdn_BufferRegion* region,
        Obdn_Image* image)
{
    Command cmd = obdn_CreateCommand(image->pChain->memory->instance, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_BeginCommandBuffer(cmd.buffer);

    VkImageLayout origLayout = image->layout;

    Barrier barrier = {
        .srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .srcAccessMask = 0,
        .dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, 
    };

    if (origLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        obdn_CmdTransitionImageLayout(cmd.buffer, barrier, image->layout, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image->mipLevels, image->handle);

    obdn_CmdCopyBufferToImage(cmd.buffer, 0, region, image);

    barrier.srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    barrier.dstAccessMask = 0;

    if (origLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        obdn_CmdTransitionImageLayout(cmd.buffer, barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, origLayout, 
                image->mipLevels, image->handle);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitAndWait(&cmd, 0);

    obdn_DestroyCommand(cmd);

    DPRINT("Copying complete.\n");
}

void obdn_LoadImage(Obdn_Memory* memory, const char* filename, const uint8_t channelCount, const VkFormat format,
    VkImageUsageFlags usageFlags,
    const VkImageAspectFlags aspectMask,
    const VkSampleCountFlags sampleCount,
    const VkFilter filter,
    const VkImageLayout layout,
    const bool createMips,
    Obdn_MemoryType memoryType,
    Image* image)
{
    assert(channelCount < 5);
    int w, h, n;
    unsigned char* data = stbi_load(filename, &w, &h, &n, channelCount);
    assert(data);
    assert(image);
    assert(image->size == 0);
    const uint32_t mipLevels = createMips ? floor(log2(fmax(w, h))) + 1 : 1;

    usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; //to copy buffer to it 
    if (createMips) 
        usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    *image = obdn_CreateImageAndSampler(memory, w, h, format, usageFlags, aspectMask, sampleCount, mipLevels, filter, memoryType);

    BufferRegion stagingBuffer = obdn_RequestBufferRegion(memory, image->size, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, OBDN_MEMORY_HOST_GRAPHICS_TYPE); //TODO: support transfer queue here

    DPRINT("loading image: width %d height %d channels %d\n", w, h, channelCount);
    DPRINT("Obdn_V_Image size: %ld\n", image->size);
    memcpy(stagingBuffer.hostData, data, w*h*channelCount);

    stbi_image_free(data);

    Command cmd = obdn_CreateCommand(memory->instance, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_BeginCommandBuffer(cmd.buffer);

    Barrier barrier = {
        .srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT
    };

    obdn_CmdTransitionImageLayout(cmd.buffer, barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, image->handle);

    obdn_CmdCopyBufferToImage(cmd.buffer, 0, &stagingBuffer, image);


    if (!createMips)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        barrier.srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        obdn_CmdTransitionImageLayout(cmd.buffer, barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout, mipLevels, image->handle);
        image->layout = layout;
    }

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitAndWait(&cmd, 0);

    obdn_FreeBufferRegion(&stagingBuffer);

    obdn_DestroyCommand(cmd);

    if (createMips)
        createMipMaps(memory->instance, VK_FILTER_LINEAR, layout, image);
}

void obdn_SaveImage(Obdn_Memory* memory, Obdn_Image* image, Obdn_V_ImageFileType fileType, const char* filename)
{
    Obdn_BufferRegion region = obdn_RequestBufferRegion(memory, 
            image->size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, OBDN_MEMORY_HOST_TRANSFER_TYPE);

    VkImageLayout origLayout = image->layout;
    obdn_TransitionImageLayout(image->layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image);

    const VkImageSubresourceLayers subRes = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .layerCount = 1, 
        .mipLevel = 0,
    };

    const VkOffset3D imgOffset = {
        .x = 0,
        .y = 0,
        .z = 0
    };

    DPRINT("Extent: %d, %d", image->extent.width, image->extent.height); 
    DPRINT("Image Layout: %d", image->layout);
    DPRINT("Orig Layout: %d", origLayout);
    DPRINT("Image size: %ld", image->size);

    const VkBufferImageCopy imgCopy = {
        .imageOffset = imgOffset,
        .imageExtent = image->extent,
        .imageSubresource = subRes,
        .bufferOffset = region.offset,
        .bufferImageHeight = 0,
        .bufferRowLength = 0
    };

    Obdn_Command cmd = obdn_CreateCommand(memory->instance, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_BeginCommandBuffer(cmd.buffer);

    DPRINT("Copying image to host...\n");
    vkCmdCopyImageToBuffer(cmd.buffer, image->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, region.buffer, 1, &imgCopy);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitAndWait(&cmd, 0);

    obdn_DestroyCommand(cmd);

    obdn_TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, origLayout, image);

    DPRINT("Copying complete.\n");
    obdn_Announce("Writing out to jpg...\n");

    char strbuf[256];
    const char* pwd = getenv("PWD");
    assert(pwd);
    const char* dir = "/";
    assert(strlen(pwd) + strlen(dir) + strlen(filename) < 256);
    strcpy(strbuf, pwd);
    strcat(strbuf, dir);
    strcat(strbuf, filename);

    DPRINT("Filepath: %s\n", strbuf);

    int r;
    switch (fileType)
    {
        case OBDN_V_IMAGE_FILE_TYPE_PNG: 
        {
            r = stbi_write_png(strbuf, image->extent.width, image->extent.height, 
            4, region.hostData, 0);
            break;
        }
        case OBDN_V_IMAGE_FILE_TYPE_JPG:
        {
            r = stbi_write_jpg(strbuf, image->extent.width, image->extent.height, 
            4, region.hostData, 0);
            break;
        }
    }

    assert( 0 != r );

    obdn_FreeBufferRegion(&region);

    obdn_Announce("Image saved to %s!\n", strbuf);
}

void obdn_v_ClearColorImage(Obdn_Image* image)
{
    Obdn_Command cmd = obdn_CreateCommand(image->pChain->memory->instance, OBDN_V_QUEUE_GRAPHICS_TYPE);

    obdn_BeginCommandBuffer(cmd.buffer);

    VkClearColorValue clearColor = {
        .float32[0] = 0,
        .float32[1] = 0,
        .float32[2] = 0,
        .float32[3] = 0,
    };

    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .layerCount = 1,
        .levelCount = 1
    };

    vkCmdClearColorImage(cmd.buffer, image->handle, image->layout, &clearColor, 1, &range);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitAndWait(&cmd, 0);

    obdn_DestroyCommand(cmd);
}
