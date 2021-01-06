#ifndef TANTO_V_MEMORY_H
#define TANTO_V_MEMORY_H

#include "v_def.h"
#include <stdbool.h>

typedef uint32_t Tanto_V_BlockId;

typedef enum {
    TANTO_V_MEMORY_HOST_TYPE,
    TANTO_V_MEMORY_DEVICE_TYPE,
} Tanto_V_MemoryType;

struct BlockChain;

typedef struct {
    VkDeviceSize           size;
    VkDeviceSize           offset;
    VkBuffer               buffer;
    uint8_t*               hostData; // if hostData not null, its mapped
    Tanto_V_BlockId        memBlockId;
    struct BlockChain*     pChain;
} Tanto_V_BufferRegion;

typedef struct {
    VkImage           handle;
    VkImageView       view;
    VkSampler         sampler;
    VkDeviceSize      size; // size in bytes. taken from GetMemReqs
    VkExtent3D        extent;
    Tanto_V_BlockId   memBlockId;
    VkImageLayout     layout;
} Tanto_V_Image;

void tanto_v_InitMemory(void);

Tanto_V_BufferRegion tanto_v_RequestBufferRegion(size_t size, 
        const VkBufferUsageFlags, const Tanto_V_MemoryType);

Tanto_V_BufferRegion tanto_v_RequestBufferRegionAligned(
        const size_t size, uint32_t alignment, const Tanto_V_MemoryType);

uint32_t tanto_v_GetMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags properties);

Tanto_V_Image tanto_v_CreateImage(
        const uint32_t width, 
        const uint32_t height,
        const VkFormat format,
        const VkImageUsageFlags usageFlags,
        const VkImageAspectFlags aspectMask,
        const VkSampleCountFlags sampleCount,
        const Tanto_V_MemoryType memType);

void tanto_v_CopyBufferRegion(const Tanto_V_BufferRegion* src, Tanto_V_BufferRegion* dst);

void tanto_v_TransferToDevice(Tanto_V_BufferRegion* pRegion);

void tanto_v_FreeImage(Tanto_V_Image* image);

void tanto_v_FreeBufferRegion(Tanto_V_BufferRegion* pRegion);

void tanto_v_CleanUpMemory(void);

// application's job to destroy this buffer and free the memory
void tanto_v_CreateUnmanagedBuffer(const VkBufferUsageFlags bufferUsageFlags, 
        const uint32_t memorySize, const Tanto_V_MemoryType type, 
        VkDeviceMemory* pMemory, VkBuffer* pBuffer);

#endif /* end of include guard: V_MEMORY_H */
