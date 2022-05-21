#include "image.h"
#include "command.h"
#include "common.h"
#include "dtags.h"
#include "memory.h"
#include "private.h"
#include "video.h"
#include <hell/debug.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_MALLOC hell_Malloc
#define STBIW_REALLOC hell_Realloc
#define STBIW_FREE hell_Free
#include "stb_image.h"
#include "stb_image_write.h"

#define IMG_OUT_DIR "/out/images/"

typedef Onyx_Command      Command;
typedef Onyx_Barrier      Barrier;
typedef Onyx_BufferRegion BufferRegion;
typedef Onyx_Image        Image;

#define DPRINT(fmt, ...) hell_DebugPrint(ONYX_DEBUG_TAG_IMG, fmt, ##__VA_ARGS__)

static void
createMipMaps(const Onyx_Instance* intstance, const VkFilter filter,
              const VkImageLayout finalLayout, Image* image)
{
    DPRINT("Creating mips for image %p\n", image->handle);

    Command cmd = onyx_CreateCommand(intstance, ONYX_V_QUEUE_GRAPHICS_TYPE);

    onyx_BeginCommandBuffer(cmd.buffer);

    VkImageMemoryBarrier barrier = {
        .sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image                       = image->handle,
        .srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1,
        .subresourceRange.levelCount     = 1,
    };

    const uint32_t mipLevels = image->mipLevels;

    uint32_t mipWidth  = image->extent.width;
    uint32_t mipHeight = image->extent.height;

    for (int i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0,
                             NULL, 1, &barrier);

        const VkImageBlit blit = {
            .srcOffsets     = {{0, 0, 0}, {mipWidth, mipHeight, 1}},
            .dstOffsets     = {{0, 0, 0},
                           {mipWidth > 1 ? mipWidth / 2 : 1,
                            mipHeight > 1 ? mipHeight / 2 : 1, 1}},
            .srcSubresource = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                               .mipLevel       = i - 1,
                               .baseArrayLayer = 0,
                               .layerCount     = 1},
            .dstSubresource = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                               .mipLevel       = i,
                               .baseArrayLayer = 0,
                               .layerCount     = 1}};

        vkCmdBlitImage(cmd.buffer, image->handle,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->handle,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = finalLayout,
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cmd.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL,
                             0, NULL, 1, &barrier);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    // last one
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = finalLayout;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;

    vkCmdPipelineBarrier(cmd.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
                         NULL, 1, &barrier);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_DestroyCommand(cmd);

    image->layout = finalLayout;
}

VkImageBlit
onyx_ImageBlitSimpleColor(uint32_t srcWidth, uint32_t srcHeight,
                          uint32_t dstWidth, uint32_t dstHeight,
                          uint32_t srcMipLevel, uint32_t dstMipLevel)
{
    VkImageBlit blit = {
        .srcOffsets     = {{0, 0, 0}, {srcWidth, srcHeight, 1}},
        .dstOffsets     = {{0, 0, 0}, {dstWidth, dstHeight, 1}},
        .srcSubresource = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                           .mipLevel       = srcMipLevel,
                           .baseArrayLayer = 0,
                           .layerCount     = 1},
        .dstSubresource = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                           .mipLevel       = dstMipLevel,
                           .baseArrayLayer = 0,
                           .layerCount     = 1}};
    return blit;
}

VkImageCopy
onyx_ImageCopySimpleColor(uint32_t width, uint32_t height, uint32_t srcOffsetX,
                          uint32_t srcOffsetY, uint32_t srcMipLevel,
                          uint32_t dstOffsetX, uint32_t dstOffsetY,
                          uint32_t dstMipLevel)
{
    VkImageCopy copy = {
        .srcSubresource = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseArrayLayer = 0,
                           .layerCount     = 1,
                           .mipLevel       = srcMipLevel},
        .srcOffset      = {.x = srcOffsetX, .y = srcOffsetY, .z = 0},
        .dstSubresource = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseArrayLayer = 0,
                           .layerCount     = 1,
                           .mipLevel       = dstMipLevel},
        .dstOffset =
            {
                .x = dstOffsetX,
                .y = dstOffsetY,
                .z = 0,
            },
        .extent = {.width = width, .height = height, .depth = 1}};
    return copy;
}

Onyx_Image
onyx_CreateImageAndSampler(Onyx_Memory* memory, const uint32_t width,
                           const uint32_t height, const VkFormat format,
                           const VkImageUsageFlags  usageFlags,
                           const VkImageAspectFlags aspectMask,
                           const VkSampleCountFlags sampleCount,
                           const uint32_t mipLevels, const VkFilter filter,
                           const Onyx_MemoryType memType)
{
    Onyx_Image image =
        onyx_CreateImage(memory, width, height, format, usageFlags, aspectMask,
                         sampleCount, mipLevels, memType);

    VkSamplerCreateInfo samplerInfo = {
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = filter,
        .minFilter    = filter,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias   = 0, // TODO understand the actual lod calculation. -0.5
                           // just seems to look better to me
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy    = 1.0,
        .compareEnable    = VK_FALSE,
        .minLod           = 0.0,
        .maxLod =
            mipLevels, // must both be 0 when using unnormalizedCoordinates
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates =
            VK_FALSE // allow us to window coordinates in frag shader
    };

    V_ASSERT(vkCreateSampler(memory->instance->device, &samplerInfo, NULL,
                             &image.sampler));

    return image;
}

void
onyx_CmdTransitionImageLayout(const VkCommandBuffer cmdbuf,
                              const Barrier         barrier,
                              const VkImageLayout   oldLayout,
                              const VkImageLayout   newLayout,
                              const uint32_t mipLevels, VkImage image)
{
    VkImageSubresourceRange subResRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel   = 0,
        .levelCount     = mipLevels,
        .layerCount     = 1,
    };

    VkImageMemoryBarrier imgBarrier = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image            = image,
        .oldLayout        = oldLayout,
        .newLayout        = newLayout,
        .subresourceRange = subResRange,
        .srcAccessMask    = barrier.srcAccessMask,
        .dstAccessMask    = barrier.dstAccessMask,
    };

    vkCmdPipelineBarrier(cmdbuf, barrier.srcStageFlags, barrier.dstStageFlags,
                         0, 0, NULL, 0, NULL, 1, &imgBarrier);
}

void
onyx_CmdCopyBufferToImage(VkCommandBuffer cmdbuf, uint32_t mipLevel,
                          const Onyx_BufferRegion* region, Onyx_Image* image)
{
    const VkImageSubresourceLayers subRes = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .layerCount     = 1,
        .mipLevel       = mipLevel,
    };

    const VkOffset3D imgOffset = {.x = 0, .y = 0, .z = 0};

    const VkBufferImageCopy imgCopy = {.imageOffset       = imgOffset,
                                       .imageExtent       = image->extent,
                                       .imageSubresource  = subRes,
                                       .bufferOffset      = region->offset,
                                       .bufferImageHeight = 0,
                                       .bufferRowLength   = 0};

    DPRINT("Copying buffer to image...\n");
    vkCmdCopyBufferToImage(cmdbuf, region->buffer, image->handle,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imgCopy);
}

void
onyx_CmdCopyImageToBuffer(VkCommandBuffer cmdbuf, uint32_t miplevel,
                          const Onyx_Image* image, Onyx_BufferRegion* region)
{
    assert(miplevel < image->mipLevels);
    const VkImageSubresourceLayers subRes = {.aspectMask = image->aspectMask,
                                             .baseArrayLayer = 0,
                                             .layerCount     = 1,
                                             .mipLevel       = miplevel};

    const VkBufferImageCopy imgCopy = {.imageOffset       = {0, 0, 0},
                                       .imageExtent       = image->extent,
                                       .imageSubresource  = subRes,
                                       .bufferOffset      = region->offset,
                                       .bufferImageHeight = 0,
                                       .bufferRowLength   = 0};

    vkCmdCopyImageToBuffer(cmdbuf, image->handle,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, region->buffer,
                           1, &imgCopy);
}

void
onyx_TransitionImageLayout(const VkImageLayout oldLayout,
                           const VkImageLayout newLayout, Onyx_Image* image)
{
    Command cmd = onyx_CreateCommand(image->pChain->memory->instance,
                                     ONYX_V_QUEUE_GRAPHICS_TYPE);

    onyx_BeginCommandBuffer(cmd.buffer);

    Barrier barrier = {
        .srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .srcAccessMask = 0,
        .dstStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = 0,
    };

    onyx_CmdTransitionImageLayout(cmd.buffer, barrier, oldLayout, newLayout,
                                  image->mipLevels, image->handle);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_DestroyCommand(cmd);

    image->layout = newLayout;
}

void
onyx_CopyBufferToImage(const Onyx_BufferRegion* region, Onyx_Image* image)
{
    Command cmd = onyx_CreateCommand(image->pChain->memory->instance,
                                     ONYX_V_QUEUE_GRAPHICS_TYPE);

    onyx_BeginCommandBuffer(cmd.buffer);

    VkImageLayout origLayout = image->layout;

    Barrier barrier = {
        .srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .srcAccessMask = 0,
        .dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
    };

    if (origLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        onyx_CmdTransitionImageLayout(cmd.buffer, barrier, image->layout,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      image->mipLevels, image->handle);

    onyx_CmdCopyBufferToImage(cmd.buffer, 0, region, image);

    barrier.srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    barrier.dstAccessMask = 0;

    if (origLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        onyx_CmdTransitionImageLayout(
            cmd.buffer, barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            origLayout, image->mipLevels, image->handle);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_DestroyCommand(cmd);

    DPRINT("Copying complete.\n");
}

uint32_t
onyx_CalcMipLevelsForImage(uint32_t width, uint32_t height)
{
    return floor(log2(fmax(width, height))) + 1;
}

void
onyx_LoadImageData(Onyx_Memory* memory, int w, int h, uint8_t channelCount,
                   void* data, const VkFormat format,
                   VkImageUsageFlags        usageFlags,
                   const VkImageAspectFlags aspectMask,
                   const VkSampleCountFlags sampleCount, const VkFilter filter,
                   const VkImageLayout layout, const bool createMips,
                   Onyx_MemoryType memoryType, Image* image)
{
    assert(data);
    const uint32_t mipLevels = createMips ? floor(log2(fmax(w, h))) + 1 : 1;

    usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // to copy buffer to it
    if (createMips)
        usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    *image =
        onyx_CreateImageAndSampler(memory, w, h, format, usageFlags, aspectMask,
                                   sampleCount, mipLevels, filter, memoryType);

    BufferRegion stagingBuffer = onyx_RequestBufferRegion(
        memory, image->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        ONYX_MEMORY_HOST_GRAPHICS_TYPE); // TODO: support transfer queue here

    DPRINT("loading image: width %d height %d channels %d\n", w, h,
           channelCount);
    DPRINT("Onyx_V_Image size: %ld\n", image->size);
    memcpy(stagingBuffer.hostData, data, w * h * channelCount);

    Command cmd =
        onyx_CreateCommand(memory->instance, ONYX_V_QUEUE_GRAPHICS_TYPE);

    onyx_BeginCommandBuffer(cmd.buffer);

    Barrier barrier = {.srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       .dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT,
                       .srcAccessMask = 0,
                       .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT};

    onyx_CmdTransitionImageLayout(
        cmd.buffer, barrier, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, image->handle);

    onyx_CmdCopyBufferToImage(cmd.buffer, 0, &stagingBuffer, image);

    if (!createMips)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        barrier.srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        onyx_CmdTransitionImageLayout(cmd.buffer, barrier,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      layout, mipLevels, image->handle);
        image->layout = layout;
    }

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_FreeBufferRegion(&stagingBuffer);

    onyx_DestroyCommand(cmd);

    if (createMips)
        createMipMaps(memory->instance, VK_FILTER_LINEAR, layout, image);
}

void
onyx_LoadImage(Onyx_Memory* memory, const char* filename,
               const uint8_t channelCount, const VkFormat format,
               VkImageUsageFlags        usageFlags,
               const VkImageAspectFlags aspectMask,
               const VkSampleCountFlags sampleCount, const VkFilter filter,
               const VkImageLayout layout, const bool createMips,
               Onyx_MemoryType memoryType, Image* image)
{
    assert(channelCount < 5);
    int            w, h, n;
    unsigned char* data = stbi_load(filename, &w, &h, &n, channelCount);
    if (!data)
    {
        hell_Error(HELL_ERR_FATAL, "stb: %s\n", stbi_failure_reason());
    }
    assert(image);
    assert(image->size == 0);
    onyx_LoadImageData(memory, w, h, channelCount, data, format, usageFlags,
                       aspectMask, sampleCount, filter, layout, createMips,
                       memoryType, image);

    stbi_image_free(data);
}

int
onyx_copy_image_to_buffer(Onyx_Image* restrict image, VkImageLayout orig_layout, Onyx_BufferRegion* restrict region)
{
    Onyx_Memory* memory = image->pChain->memory;
    *region = onyx_RequestBufferRegion(
        memory, image->size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        ONYX_MEMORY_HOST_TRANSFER_TYPE);

    onyx_TransitionImageLayout(orig_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image);

    const VkImageSubresourceLayers subRes = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .layerCount     = 1,
        .mipLevel       = 0,
    };

    const VkOffset3D imgOffset = {.x = 0, .y = 0, .z = 0};

    DPRINT("Extent: %d, %d", image->extent.width, image->extent.height);
    DPRINT("Image Layout: %d", image->layout);
    DPRINT("Orig Layout: %d", orig_layout);
    DPRINT("Image size: %ld", image->size);

    const VkBufferImageCopy imgCopy = {.imageOffset       = imgOffset,
                                       .imageExtent       = image->extent,
                                       .imageSubresource  = subRes,
                                       .bufferOffset      = region->offset,
                                       .bufferImageHeight = 0,
                                       .bufferRowLength   = 0};

    Onyx_Command cmd =
        onyx_CreateCommand(memory->instance, ONYX_V_QUEUE_GRAPHICS_TYPE);

    onyx_BeginCommandBuffer(cmd.buffer);

    DPRINT("Copying image to host...\n");
    vkCmdCopyImageToBuffer(cmd.buffer, image->handle,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, region->buffer,
                           1, &imgCopy);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_DestroyCommand(cmd);

    onyx_TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, orig_layout,
                               image);

    return 0;
}

int 
onyx_copy_png_buf_to_image(const uint8_t* restrict png_buf,
                            const int png_buf_size,
                            Onyx_Memory* memory,
                            VkImageLayout layout,
                            Onyx_Image* restrict img)
{
    int x,y,comp;
    uint8_t* pxls = stbi_load_from_memory(png_buf, png_buf_size, &x, &y, &comp, 0);
    if (!pxls)
        return -1;

    assert((uint32_t)x == img->extent.width);
    assert((uint32_t)y == img->extent.height);
    assert(x * y * comp == (int)img->size);

    Onyx_BufferRegion gpu_buf = onyx_RequestBufferRegion(
        memory, img->size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, ONYX_MEMORY_HOST_TRANSFER_TYPE);

    memcpy(gpu_buf.hostData, pxls, img->size);

    // copy buffer to image should take layout parm
    assert(img->layout == layout);
    onyx_CopyBufferToImage(&gpu_buf, img);

    onyx_FreeBufferRegion(&gpu_buf);
    hell_Free(pxls);

    return 0;
}

int
onyx_write_image_to_png_buf(Onyx_Image* restrict img,
                            VkImageLayout layout,
                            uint8_t** restrict png_buf,
                            int* restrict png_buf_size)
{
    Onyx_BufferRegion br = {0};
    int err;

    err = onyx_copy_image_to_buffer(img, layout, &br);
    if (err)
        goto end;

    int ncomp = 0;
    switch (img->format)
    {
    case (VK_FORMAT_R8_UNORM):
    case (VK_FORMAT_R8_SNORM):
    case (VK_FORMAT_R8_USCALED):
    case (VK_FORMAT_R8_SSCALED):
    case (VK_FORMAT_R8_UINT):
    case (VK_FORMAT_R8_SINT):
    case (VK_FORMAT_R8_SRGB):
        ncomp = 1;
        break;
    case (VK_FORMAT_R8G8B8A8_UNORM):
    case (VK_FORMAT_R8G8B8A8_SNORM):
    case (VK_FORMAT_R8G8B8A8_USCALED):
    case (VK_FORMAT_R8G8B8A8_SSCALED):
    case (VK_FORMAT_R8G8B8A8_UINT):
    case (VK_FORMAT_R8G8B8A8_SINT):
    case (VK_FORMAT_R8G8B8A8_SRGB):
    case (VK_FORMAT_B8G8R8A8_UNORM):
    case (VK_FORMAT_B8G8R8A8_SNORM):
    case (VK_FORMAT_B8G8R8A8_USCALED):
    case (VK_FORMAT_B8G8R8A8_SSCALED):
    case (VK_FORMAT_B8G8R8A8_UINT):
    case (VK_FORMAT_B8G8R8A8_SINT):
    case (VK_FORMAT_B8G8R8A8_SRGB):
        ncomp = 4;
        break;
    default:
        err = -1;
        goto end;
    }

    assert(ncomp && "Image format is not being handled in switch");

    const uint8_t* pixels = br.hostData;
    *png_buf = stbi_write_png_to_mem(pixels, 0, img->extent.width, img->extent.height, ncomp, png_buf_size);
    if (!png_buf) {err = -1; goto end;}

end:
    onyx_FreeBufferRegion(&br);
    return err;
}

void
onyx_SaveImage(Onyx_Memory* memory, Onyx_Image* image,
               Onyx_V_ImageFileType fileType, VkImageLayout image_layout,
               const char* filename)
{
    Onyx_BufferRegion region = onyx_RequestBufferRegion(
        memory, image->size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        ONYX_MEMORY_HOST_TRANSFER_TYPE);

    VkImageLayout origLayout = image_layout;
    onyx_TransitionImageLayout(origLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image);

    const VkImageSubresourceLayers subRes = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .layerCount     = 1,
        .mipLevel       = 0,
    };

    const VkOffset3D imgOffset = {.x = 0, .y = 0, .z = 0};

    DPRINT("Extent: %d, %d", image->extent.width, image->extent.height);
    DPRINT("Image Layout: %d", image->layout);
    DPRINT("Orig Layout: %d", origLayout);
    DPRINT("Image size: %ld", image->size);

    const VkBufferImageCopy imgCopy = {.imageOffset       = imgOffset,
                                       .imageExtent       = image->extent,
                                       .imageSubresource  = subRes,
                                       .bufferOffset      = region.offset,
                                       .bufferImageHeight = 0,
                                       .bufferRowLength   = 0};

    Onyx_Command cmd =
        onyx_CreateCommand(memory->instance, ONYX_V_QUEUE_GRAPHICS_TYPE);

    onyx_BeginCommandBuffer(cmd.buffer);

    DPRINT("Copying image to host...\n");
    vkCmdCopyImageToBuffer(cmd.buffer, image->handle,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, region.buffer,
                           1, &imgCopy);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_DestroyCommand(cmd);

    onyx_TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, origLayout,
                               image);

    DPRINT("Copying complete.\n");
    onyx_Announce("Writing out to jpg...\n");

    char        strbuf[256];
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
    case ONYX_V_IMAGE_FILE_TYPE_PNG: {
        r = stbi_write_png(strbuf, image->extent.width, image->extent.height, 4,
                           region.hostData, 0);
        break;
    }
    case ONYX_V_IMAGE_FILE_TYPE_JPG: {
        r = stbi_write_jpg(strbuf, image->extent.width, image->extent.height, 4,
                           region.hostData, 0);
        break;
    }
    }

    assert(0 != r);

    onyx_FreeBufferRegion(&region);

    onyx_Announce("Image saved to %s!\n", strbuf);
}

void
onyx_v_ClearColorImage(Onyx_Image* image)
{
    Onyx_Command cmd = onyx_CreateCommand(image->pChain->memory->instance,
                                          ONYX_V_QUEUE_GRAPHICS_TYPE);

    onyx_BeginCommandBuffer(cmd.buffer);

    VkClearColorValue clearColor = {
        .float32[0] = 0,
        .float32[1] = 0,
        .float32[2] = 0,
        .float32[3] = 0,
    };

    VkImageSubresourceRange range = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                     .baseArrayLayer = 0,
                                     .baseMipLevel   = 0,
                                     .layerCount     = 1,
                                     .levelCount     = 1};

    vkCmdClearColorImage(cmd.buffer, image->handle, image->layout, &clearColor,
                         1, &range);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_DestroyCommand(cmd);
}

VkImageSubresourceRange
onyx_GetImageSubresourceRange(const Onyx_Image* img)
{
    return onyx_ImageSubresourceRange(img->aspectMask, 0, img->mipLevels, 0, 0);
}
