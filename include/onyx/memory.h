#ifndef ONYX_V_MEMORY_H
#define ONYX_V_MEMORY_H

#include "video.h"
#include <stdbool.h>


// TODO this is brittle. we should probably change V_MemoryType to a mask with
// flags for the different
// requirements. One bit for host or device, one bit for transfer capabale, one
// bit for external, one bit for image or buffer
typedef enum {
    ONYX_MEMORY_HOST_GRAPHICS_TYPE,
    ONYX_MEMORY_HOST_TRANSFER_TYPE,
    ONYX_MEMORY_DEVICE_TYPE,
    ONYX_MEMORY_EXTERNAL_DEVICE_TYPE
} Onyx_MemoryType;

typedef struct Onyx_Memory Onyx_Memory;
typedef struct Onyx_Memory onyx_Memory;
typedef Onyx_MemoryType onyx_MemoryType;

struct BlockChain;

typedef struct Onyx_BufferRegion {
    VkDeviceSize       size;
    VkDeviceSize       offset;
    VkBuffer           buffer;
    uint8_t*           hostData; // if hostData not null, its mapped
    uint32_t           memBlockId;
    uint32_t           stride; // if bufferregion stores an array this is the stride between elements. otherwise its 0. if the elements need to satify an alignment this stride will satisfy that
    struct BlockChain* pChain;
} Onyx_BufferRegion;

typedef struct Onyx_Image {
    VkImage            handle;
    VkImageView        view;
    VkSampler          sampler;
    VkImageAspectFlags aspectMask;
    VkDeviceSize       size; // size in bytes. taken from GetMemReqs
    VkDeviceSize       offset;//TODO: delete this and see if its even necessary... should probably be a VkOffset3D object
    VkExtent3D         extent;
    VkImageLayout      layout;
    VkImageUsageFlags  usageFlags;
    VkFormat           format;
    uint32_t           mipLevels;
    uint32_t           queueFamily;
    uint32_t           memBlockId;
    struct BlockChain* pChain;
} Onyx_Image;

uint64_t onyx_SizeOfMemory(void);
Onyx_Memory* onyx_AllocMemory(void);
void onyx_CreateMemory(const Onyx_Instance* instance, const uint32_t hostGraphicsBufferMB,
                  const uint32_t deviceGraphicsBufferMB,
                  const uint32_t deviceGraphicsImageMB, const uint32_t hostTransferBufferMB,
                  const uint32_t deviceExternalGraphicsImageMB, Onyx_Memory* memory);

Onyx_BufferRegion onyx_RequestBufferRegion(Onyx_Memory*, size_t size,
                                             const VkBufferUsageFlags,
                                             const Onyx_MemoryType);

// Returns a buffer region with enough space for elemCount elements of elemSize size. 
// The stride member set to the size of the space between
// elements, which might be different from elemSize if physicalDevice requirements demand it.
Onyx_BufferRegion
onyx_RequestBufferRegionArray(Onyx_Memory* memory, uint32_t elemSize, uint32_t elemCount,
                              VkBufferUsageFlags flags,
                              Onyx_MemoryType  memType);

Onyx_BufferRegion onyx_RequestBufferRegionAligned(Onyx_Memory*, const size_t size,
                                                    uint32_t     alignment,
                                                    const Onyx_MemoryType);

uint32_t onyx_GetMemoryType(const Onyx_Memory*, uint32_t                    typeBits,
                            const VkMemoryPropertyFlags properties);

Onyx_Image onyx_CreateImage(Onyx_Memory*, const uint32_t width, const uint32_t height,
                              const VkFormat           format,
                              const VkImageUsageFlags  usageFlags,
                              const VkImageAspectFlags aspectMask,
                              const VkSampleCountFlags sampleCount,
                              const uint32_t           mipLevels,
                              const Onyx_MemoryType);

void onyx_CopyBufferRegion(const Onyx_BufferRegion* src,
                        Onyx_BufferRegion*       dst);

void onyx_CopyImageToBufferRegion(const Onyx_Image*  image,
                                  Onyx_BufferRegion* bufferRegion);

void onyx_TransferToDevice(Onyx_Memory* memory, Onyx_BufferRegion* pRegion);

void onyx_FreeImage(Onyx_Image* image);

void onyx_FreeBufferRegion(Onyx_BufferRegion* pRegion);

void onyx_DestroyMemory(Onyx_Memory* memory);

void onyx_MemoryReportSimple(const Onyx_Memory* memory);

VkDeviceAddress onyx_GetBufferRegionAddress(const Onyx_BufferRegion* region);

// application's job to destroy this buffer and free the memory
void onyx_CreateUnmanagedBuffer(Onyx_Memory* memory, const VkBufferUsageFlags bufferUsageFlags,
                                const uint32_t           memorySize,
                                const Onyx_MemoryType  type,
                                VkDeviceMemory* pMemory, VkBuffer* pBuffer);

VkDeviceMemory onyx_GetDeviceMemory(const Onyx_Memory* memory, const Onyx_MemoryType memType);
VkDeviceSize   onyx_GetMemorySize(const Onyx_Memory* memory, const Onyx_MemoryType memType);

const Onyx_Instance* onyx_GetMemoryInstance(const Onyx_Memory* memory);

void onyx_GetImageMemoryUsage(const Onyx_Memory* memory, uint64_t* bytes_in_use, uint64_t* total_bytes);

#ifdef WIN32
bool onyx_GetExternalMemoryWin32Handle(const Onyx_Memory* memory, HANDLE* handle, uint64_t* size);
#else
bool onyx_GetExternalMemoryFd(const Onyx_Memory* memory, int* fd, uint64_t* size);
#endif

void
onyx_ResizeBufferRegion(Onyx_BufferRegion* region, size_t new_size);

#endif /* end of include guard: V_MEMORY_H */
