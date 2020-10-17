#ifndef TANTO_V_MEMORY_H
#define TANTO_V_MEMORY_H

#include "v_def.h"
#include <stdbool.h>

typedef uint32_t Tanto_V_BlockId;

//typedef struct {
//    size_t       size;
//    uint8_t*     hostData;
//    VkDeviceSize vOffset;
//    VkBuffer*    vBuffer;
//    bool         isMapped;
//} Tanto_V_BlockHostBuffer;

typedef enum {
    TANTO_V_MEMORY_HOST_TYPE,
    TANTO_V_MEMORY_DEVICE_TYPE,
} Tanto_V_MemoryType;

typedef struct {
    VkDeviceSize    size;
    VkDeviceSize    offset;
    VkBuffer        buffer;
    uint8_t*        hostData; // if hostData not null, its mapped
    Tanto_V_BlockId memBlockId;
} Tanto_V_BufferRegion;

typedef struct {
    VkImage           handle;
    VkImageView       view;
    VkSampler         sampler;
    Tanto_V_BlockId   memBlockId;
} Tanto_V_Image;

void tanto_v_InitMemory(void);

Tanto_V_BufferRegion tanto_v_RequestBufferRegion(size_t size, 
        const VkBufferUsageFlags);

Tanto_V_BufferRegion tanto_v_RequestBufferRegionAligned(
        const size_t size, const uint32_t alignment, const Tanto_V_MemoryType);

uint32_t tanto_v_GetMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags properties);

Tanto_V_Image tanto_v_CreateImage(
        const uint32_t width, 
        const uint32_t height,
        const VkFormat format,
        const VkImageUsageFlags usageFlags,
        const VkImageAspectFlags aspectMask);

void tanto_v_DestroyImage(Tanto_V_Image image);

void tanto_v_FreeBufferRegion(Tanto_V_BufferRegion region);

void tanto_v_CleanUpMemory(void);

#endif /* end of include guard: V_MEMORY_H */
