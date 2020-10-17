#include "v_memory.h"
#include "v_video.h"
#include "t_def.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

// HVC = Host Visible and Coherent
// DL = Device Local    
#define MEMORY_SIZE_HOST        52428800  
#define MEMORY_SIZE_DEV_BUFFER  52428800  // 50 MiB
#define MEMORY_SIZE_DEV_IMAGE   524288000 // 500 MiB
#define MAX_BLOCKS 256

static VkPhysicalDeviceMemoryProperties memoryProperties;

typedef struct tanto_V_MemBlock {
    VkDeviceSize    size;
    VkDeviceSize    offset;
    bool            inUse;
    Tanto_V_BlockId id; // global unique identifier
} Tanto_V_MemBlock;

struct BlockChain {
    uint32_t          totalSize;
    uint32_t          count;
    uint32_t          cur;
    uint32_t          nextBlockId;
    VkDeviceMemory    memory;
    VkBuffer          buffer;
    uint8_t*          hostData;
    Tanto_V_MemBlock  blocks[MAX_BLOCKS];
};

static struct BlockChain hbBlockChain;
static struct BlockChain diBlockChain;
static struct BlockChain dbBlockChain;

static bool initializedBlockChain;

static void printBufferMemoryReqs(const VkMemoryRequirements* reqs)
{
    printf("Size: %ld\tAlignment: %ld\n", reqs->size, reqs->alignment);
}

static void printBlockChainInfo(const struct BlockChain* chain)
{
    printf("BlockChain Info:\n");
    printf("totalSize: %d\t count: %d\t cur: %d\t nextBlockId: %d\n", chain->totalSize, chain->count, chain->cur, chain->nextBlockId);
    printf("memory: %p\t buffer: %p\t hostData: %p\n", chain->memory, chain->buffer, chain->hostData);
    printf("Blocks: \n");
    for (int i = 0; i < chain->count; i++) 
    {
        const Tanto_V_MemBlock* block = &chain->blocks[i];
        printf("{ Block %d: size = %ld, offset = %ld, inUse = %s, id: %d}, ", i, block->size, block->offset, block->inUse ? "true" : "false", block->id);
    }
    printf("\n");
}

static void initBlockChain(const VkDeviceSize memorySize, const uint32_t memTypeIndex, const VkBufferUsageFlags bufferUsageFlags, struct BlockChain* chain)
{
    memset(chain->blocks, 0, MAX_BLOCKS * sizeof(Tanto_V_MemBlock));
    assert( memorySize % 0x40 == 0 ); // make sure memorysize is 64 byte aligned (arbitrary choice)
    chain->count = 1;
    chain->cur   = 0;
    chain->totalSize = memorySize;
    chain->nextBlockId = 0;
    chain->blocks[0].inUse = false;
    chain->blocks[0].offset = 0;
    chain->blocks[0].size = memorySize;

    const VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memorySize,
        .memoryTypeIndex = memTypeIndex 
    };

    V_ASSERT( vkAllocateMemory(device, &allocInfo, NULL, &chain->memory) ); 

    if (bufferUsageFlags)
    {
        VkBufferCreateInfo ci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .usage = bufferUsageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // queue determined by first use
            .size = memorySize 
        };

        V_ASSERT( vkCreateBuffer(device, &ci, NULL, &chain->buffer) );

        V_ASSERT( vkBindBufferMemory(device, chain->buffer, chain->memory, 0) );

        VkMemoryRequirements memReqs;

        vkGetBufferMemoryRequirements(device, chain->buffer, &memReqs);

        printf("Host Buffer ALIGNMENT: %ld\n", memReqs.alignment);

        V_ASSERT( vkMapMemory(device, chain->memory, 0, VK_WHOLE_SIZE, 0, (void**)&chain->hostData) );
    }
    else
    {
        chain->buffer = VK_NULL_HANDLE;
        chain->hostData = NULL;
    }
}

static void freeBlockChain(struct BlockChain* chain)
{
    memset(chain->blocks, 0, MAX_BLOCKS * sizeof(Tanto_V_MemBlock));
    if (chain->buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, chain->buffer, NULL);
        vkUnmapMemory(device, chain->memory);
        chain->buffer = VK_NULL_HANDLE;
    }
    vkFreeMemory(device, chain->memory, NULL);
}

static Tanto_V_MemBlock* requestBlock(const uint32_t size, const uint32_t alignment, struct BlockChain* chain)
{
    //size_t cur  = chain->cur;
    size_t cur  = 0;
    const size_t init = cur;
    const size_t count = chain->count;
    assert( size < chain->totalSize );
    assert( count > 0 );
    assert( cur < count );
    while (chain->blocks[cur].inUse || chain->blocks[cur].size < size)
    {
        cur = (cur + 1) % count;
        assert( cur != init ); // looped around. no blocks suitable.
    }
    // found a block not in use and with enough size
    // split the block
    Tanto_V_MemBlock* curBlock = &chain->blocks[cur];
    if (curBlock->size == size && curBlock->offset % alignment == 0) // just reuse this block;
    {
        printf("Re-using block\n");
        curBlock->inUse = true;
        return curBlock;
    }
    size_t new = chain->count++;
    Tanto_V_MemBlock* newBlock = &chain->blocks[new];
    assert( new < MAX_BLOCKS );
    assert( newBlock->inUse == false );
    VkDeviceSize alignedOffset = curBlock->offset;
    if (alignedOffset % alignment != 0) // not aligned
        alignedOffset = (alignedOffset / alignment + 1) * alignment;
    const VkDeviceSize offsetDiff = alignedOffset - curBlock->offset;
    // take away the size lost due to alignment and the new size
    newBlock->size   = curBlock->size - offsetDiff - size;
    newBlock->offset = alignedOffset + size;
    curBlock->size   = size;
    curBlock->offset = alignedOffset;
    curBlock->inUse = true;
    curBlock->id = chain->nextBlockId++;
    chain->cur = cur;
    return curBlock;
}

static void freeBlock(struct BlockChain* chain, const Tanto_V_BlockId id)
{
    assert( id < chain->nextBlockId);
    const size_t blockCount = chain->count;
    int blockIndex = 0;
    for ( ; blockIndex < blockCount; blockIndex++) 
    {
        if (chain->blocks[blockIndex].id == id)
            break;
    }
    assert( blockIndex < blockCount ); // block must not have come from this chain
    Tanto_V_MemBlock* block = &chain->blocks[blockIndex];
    printf("Freeing block id: %d\n", id);
    block->inUse = false;
}

void tanto_v_InitMemory(void)
{
    uint32_t hostVisibleCoherentTypeIndex;
    int deviceLocalTypeIndex;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    V1_PRINT("Memory Heap Info:\n");
    for (int i = 0; i < memoryProperties.memoryHeapCount; i++) 
    {
        V1_PRINT("Heap %d: Size %ld: %s local\n", 
                i,
                memoryProperties.memoryHeaps[i].size, 
                memoryProperties.memoryHeaps[i].flags 
                & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "Device" : "Host");
                // note there are other possible flags, but seem to only deal with multiple gpus
    }

    bool foundHvc = false;
    bool foundDl  = false;

    V1_PRINT("Memory Type Info:\n");
    for (int i = 0; i < memoryProperties.memoryTypeCount; i++) 
    {
        VkMemoryPropertyFlags flags = memoryProperties.memoryTypes[i].propertyFlags;
        V1_PRINT("Type %d: Heap Index: %d Flags: | %s%s%s%s%s%s\n", 
                i, 
                memoryProperties.memoryTypes[i].heapIndex,
                flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ?  "Device Local | " : "",
                flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ?  "Host Visible | " : "",
                flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "Host Coherent | " : "",
                flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ?   "Host Cached | " : "",
                flags & VK_MEMORY_PROPERTY_PROTECTED_BIT ?     "Protected | "   : "",
                flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ? "Lazily allocated | " : ""
                );   
        if ((flags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) &&
                !foundHvc)
        {
            hostVisibleCoherentTypeIndex = i;
            foundHvc = true;
        }
        if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !foundDl)
        {
            deviceLocalTypeIndex = i;
            foundDl = true;
        }
    }

    assert( foundHvc );
    assert( foundDl );

    VkBufferUsageFlags bhbFlags = 
         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
         VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
         VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR;

    initBlockChain(MEMORY_SIZE_HOST, hostVisibleCoherentTypeIndex, bhbFlags, &hbBlockChain);
    initBlockChain(MEMORY_SIZE_DEV_IMAGE, deviceLocalTypeIndex, 0, &diBlockChain);
}

Tanto_V_BufferRegion tanto_v_RequestBufferRegionAligned(
        const size_t size, 
        const uint32_t alignment, const Tanto_V_MemoryType memType)
{
    assert( size % 4 == 0 ); // only allow for word-sized blocks
    Tanto_V_MemBlock* block;
    switch (memType) 
    {
        case TANTO_V_MEMORY_HOST_TYPE: block = requestBlock(size, alignment, &hbBlockChain); break;
        case TANTO_V_MEMORY_DEVICE_TYPE: block = requestBlock(size, alignment, &dbBlockChain); break;
    }

    Tanto_V_BufferRegion region;
    region.offset = block->offset;
    region.memBlockId = block->id;
    region.size = block->size;

    if (memType == TANTO_V_MEMORY_HOST_TYPE)
    {
        region.buffer = hbBlockChain.buffer;
        region.hostData = hbBlockChain.hostData + block->offset;
    }
    else
    {
        region.buffer = VK_NULL_HANDLE;
        region.hostData = NULL;
    }

    printBlockChainInfo(&hbBlockChain);

    return region;
}

Tanto_V_BufferRegion tanto_v_RequestBufferRegion(const size_t size, const VkBufferUsageFlags flags)
{
    uint32_t alignment = 0x40;
    // the order of this if statements matters. it garuantees the maximum
    // alignment is chose if multiple flags are present
    if ( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT & flags)
    {
        alignment = 0x10;
        // must satisfy alignment requirements for storage buffers
    }
    if ( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT & flags)
    {
        // must satisfy alignment requirements for uniform buffers
        alignment = 0x40;
    }
    return tanto_v_RequestBufferRegionAligned(size, alignment, TANTO_V_MEMORY_HOST_TYPE);
}

uint32_t tanto_v_GetMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags properties)
{
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
      if(((typeBits & (1 << i)) > 0) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
      {
        return i;
      }
    }
    assert(0);
    return ~0u;
}

Tanto_V_Image tanto_v_CreateImage(
        const uint32_t width, 
        const uint32_t height,
        const VkFormat format,
        const VkImageUsageFlags usageFlags,
        const VkImageAspectFlags aspectMask)
{
    assert( width * height < MEMORY_SIZE_DEV_IMAGE );

    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphicsQueueFamilyIndex,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    Tanto_V_Image image;

    V_ASSERT( vkCreateImage(device, &imageInfo, NULL, &image.handle) );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, image.handle, &memReqs);

    printf("Requesting image of size %ld\n", memReqs.size);

    const Tanto_V_MemBlock* block = requestBlock(memReqs.size, memReqs.alignment, &diBlockChain);
    image.memBlockId = block->id;

    vkBindImageMemory(device, image.handle, diBlockChain.memory, block->offset);

    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .components = {0, 0, 0, 0}, // no swizzling
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    V_ASSERT( vkCreateImageView(device, &viewInfo, NULL, &image.view) );

    image.sampler = VK_NULL_HANDLE;

    return image;
}

void tanto_v_DestroyImage(Tanto_V_Image image)
{
    printf("Destroy image called\n");
    if (image.sampler != VK_NULL_HANDLE)
        vkDestroySampler(device, image.sampler, NULL);
    vkDestroyImageView(device, image.view, NULL);
    vkDestroyImage(device, image.handle, NULL);
    freeBlock(&diBlockChain, image.memBlockId);
}

void tanto_v_FreeBufferRegion(Tanto_V_BufferRegion region)
{
    freeBlock(&hbBlockChain, region.memBlockId);
}

void tanto_v_CleanUpMemory()
{
    freeBlockChain(&hbBlockChain);
    freeBlockChain(&diBlockChain);
};
