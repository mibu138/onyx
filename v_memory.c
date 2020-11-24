#include "v_memory.h"
#include "v_video.h"
#include "t_def.h"
#include "v_command.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

// HVC = Host Visible and Coherent
// DL = Device Local    
#define MEMORY_SIZE_HOST          0x40000000 // 
#define MEMORY_SIZE_DEV_BUFFER    0x40000000 // 
#define MEMORY_SIZE_DEV_IMAGE     0x20000000 // 
#define MEMORY_SIZE_HOST_TRANSFER 0x10000000 // 
#define MAX_BLOCKS 256

static VkPhysicalDeviceMemoryProperties memoryProperties;

typedef struct tanto_V_MemBlock {
    VkDeviceSize    size;
    VkDeviceSize    offset;
    bool            inUse;
    Tanto_V_BlockId id; // global unique identifier
} Tanto_V_MemBlock;

struct BlockChain {
    char               name[16]; // for debugging
    VkDeviceSize       totalSize;
    VkDeviceSize       defaultAlignment;
    uint32_t           count;
    uint32_t           cur;
    uint32_t           nextBlockId;
    VkDeviceMemory     memory;
    VkBuffer           buffer;
    VkBufferUsageFlags bufferFlags;
    uint8_t*           hostData;
    Tanto_V_MemBlock   blocks[MAX_BLOCKS];
};

static struct BlockChain blockChainHostGraphics;
static struct BlockChain blockChainHostTransfer;
static struct BlockChain blockChainDeviceGraphics;
static struct BlockChain blockChainDeviceImage;

static void printBufferMemoryReqs(const VkMemoryRequirements* reqs)
{
    printf("Size: %ld\tAlignment: %ld\n", reqs->size, reqs->alignment);
}

static void printBlockChainInfo(const struct BlockChain* chain)
{
    printf("BlockChain %s:\n", chain->name);
    printf("totalSize: %ld\t count: %d\t cur: %d\t nextBlockId: %d\n", chain->totalSize, chain->count, chain->cur, chain->nextBlockId);
    printf("memory: %p\t buffer: %p\t hostData: %p\n", chain->memory, chain->buffer, chain->hostData);
    printf("Blocks: \n");
    for (int i = 0; i < chain->count; i++) 
    {
        const Tanto_V_MemBlock* block = &chain->blocks[i];
        printf("{ Block %d: size = %ld, offset = %ld, inUse = %s, id: %d}, ", i, block->size, block->offset, block->inUse ? "true" : "false", block->id);
    }
    printf("\n");
}

static void initBlockChain(
        const VkDeviceSize memorySize, 
        const uint32_t memTypeIndex, 
        const VkBufferUsageFlags bufferUsageFlags, 
        const bool mapBuffer,
        const char* name,
        struct BlockChain* chain)
{
    memset(chain->blocks, 0, MAX_BLOCKS * sizeof(Tanto_V_MemBlock));
    assert( memorySize % 0x40 == 0 ); // make sure memorysize is 64 byte aligned (arbitrary choice)
    chain->count = 1;
    chain->cur   = 0;
    chain->totalSize = memorySize;
    chain->nextBlockId = 1; 
    chain->defaultAlignment = 4;
    chain->bufferFlags = bufferUsageFlags;
    chain->blocks[0].inUse = false;
    chain->blocks[0].offset = 0;
    chain->blocks[0].size = memorySize;
    chain->blocks[0].id = 0;
    assert( strlen(name) < 16 );
    strcpy(chain->name, name);

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

        //chain->defaultAlignment = memReqs.alignment;

        V1_PRINT("Host Buffer ALIGNMENT: %ld\n", memReqs.alignment);

        if (mapBuffer)
            V_ASSERT( vkMapMemory(device, chain->memory, 0, VK_WHOLE_SIZE, 0, (void**)&chain->hostData) );
        else 
            chain->hostData = NULL;
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

static int findAvailableBlockIndex(const uint32_t size, struct BlockChain* chain)
{
    size_t cur  = 0;
    size_t init = cur;
    const size_t count = chain->count;
    assert( size < chain->totalSize );
    assert( count > 0 );
    assert( cur < count );
    while (chain->blocks[cur].inUse || chain->blocks[cur].size < size)
    {
        cur = (cur + 1) % count;
        if (cur == init) 
        {
            V1_PRINT("%s: no suitable block found.\n", __PRETTY_FUNCTION__);
            return -1;
        }
    }
    // found a block not in use and with enough size
    return cur;
}

// from i = firstIndex to i = lastIndex - 1, swap block i with block i + 1
// note this can operate on indices beyond chain->count
static void rotateBlockUp(const size_t fromIndex, const size_t toIndex, struct BlockChain* chain)
{
    assert ( fromIndex >= 0 );
    assert ( toIndex < MAX_BLOCKS);
    assert ( toIndex > fromIndex );
    for (int i = fromIndex; i < toIndex; i++) 
    {
        Tanto_V_MemBlock temp = chain->blocks[i];
        chain->blocks[i] = chain->blocks[i + 1];
        chain->blocks[i + 1] = temp;
    }
}

static void rotateBlockDown(const size_t fromIndex, const size_t toIndex, struct BlockChain* chain)
{
    assert ( toIndex >= 0 );
    assert ( fromIndex < MAX_BLOCKS);
    assert ( fromIndex > toIndex );
    for (int i = fromIndex; i > toIndex; i--) 
    {
        Tanto_V_MemBlock temp = chain->blocks[i];
        chain->blocks[i] = chain->blocks[i - 1];
        chain->blocks[i - 1] = temp;
    }
}

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

static void mergeBlocks(struct BlockChain* chain)
{
    assert (chain->count > 1); // must have at least 2 blocks to defragment
    for (int i = 0; i < chain->count - 1; i++) 
    {
        Tanto_V_MemBlock* curr = &chain->blocks[i];   
        Tanto_V_MemBlock* next = &chain->blocks[i + 1];   
        if (!curr->inUse && !next->inUse)
        {
            // combine them together
            const VkDeviceSize cSize   = curr->size + next->size;
            // we could make the other id available for re-use as well
            curr->size   = cSize;
            // set next block to 0
            memset(next, 0, sizeof(Tanto_V_MemBlock));
            // rotate blocks down past i so that next goes to chain->count - 1
            // and the block at chain->size - 1 goes to chain->count - 2
            if (i + 1 != chain->count - 1)
            {
                rotateBlockUp(i + 1, chain->count - 1, chain);
            }
            // decrement the chain count
            chain->count--;
            i--;
        }
    }
}

// TODO: implement a proper defragment function
static void defragment(struct BlockChain* chain)
{
    V1_PRINT("Empty defragment function called\n");
}

static bool chainIsOrdered(const struct BlockChain* chain)
{
    for (int i = 0; i < chain->count - 1; i++) 
    {
        const Tanto_V_MemBlock* curr = &chain->blocks[i];
        const Tanto_V_MemBlock* next = &chain->blocks[i + 1];
        if (curr->offset > next->offset) return false;
    }
    return true;
}

static Tanto_V_MemBlock* requestBlock(const uint32_t size, const uint32_t alignment, struct BlockChain* chain)
{
    const int curIndex = findAvailableBlockIndex(size, chain);
    if (curIndex < 0) // try defragmenting. if that fails we're done.
    {
        defragment(chain); 
        const int curIndex = findAvailableBlockIndex(size, chain);
        if (curIndex < 0)
        {
            printf("Memory allocation failed\n");
            exit(0);
        }
    }
    Tanto_V_MemBlock* curBlock = &chain->blocks[curIndex];
    if (curBlock->size == size && curBlock->offset % alignment == 0) // just reuse this block;
    {
        curBlock->inUse = true;
        return curBlock;
    }
    // split the block
    const size_t newIndex = chain->count++;
    Tanto_V_MemBlock* newBlock = &chain->blocks[newIndex];
    assert( newIndex < MAX_BLOCKS );
    assert( newBlock->inUse == false );
    VkDeviceSize alignedOffset = curBlock->offset;
    if (alignedOffset % alignment != 0) // not aligned
        alignedOffset = (alignedOffset / alignment + 1) * alignment;
    const VkDeviceSize offsetDiff = alignedOffset - curBlock->offset;
    // take away the size lost due to alignment and the new size
    newBlock->size   = curBlock->size - offsetDiff - size;
    newBlock->offset = alignedOffset + size;
    newBlock->id = chain->nextBlockId++;
    curBlock->size   = size;
    curBlock->offset = alignedOffset;
    curBlock->inUse = true;
    if (newIndex != curIndex + 1)
        rotateBlockDown(newIndex, curIndex + 1, chain);
    assert( chainIsOrdered(chain) );
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
    block->inUse = false;
    mergeBlocks(chain);
}

void tanto_v_InitMemory(void)
{
    uint32_t hostVisibleCoherentTypeIndex = 0;
    uint32_t deviceLocalTypeIndex = 0;

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
         VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR |
         VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
         VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBufferUsageFlags hostTransferFlags = 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBufferUsageFlags devBufFlags = 
         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
         VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
         VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR |
         VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    initBlockChain(MEMORY_SIZE_HOST, hostVisibleCoherentTypeIndex, bhbFlags, true, "hostGraphic", &blockChainHostGraphics);
    initBlockChain(MEMORY_SIZE_DEV_IMAGE, deviceLocalTypeIndex, 0, false, "devImage", &blockChainDeviceImage);
    initBlockChain(MEMORY_SIZE_HOST_TRANSFER, hostVisibleCoherentTypeIndex, hostTransferFlags, true, "hostTransfer", &blockChainHostTransfer);
    initBlockChain(MEMORY_SIZE_DEV_BUFFER, deviceLocalTypeIndex, devBufFlags, false, "devGraphic", &blockChainDeviceGraphics);
}

Tanto_V_BufferRegion tanto_v_RequestBufferRegionAligned(
        const size_t size, 
        uint32_t alignment, const Tanto_V_MemoryType memType)
{
    assert( size % 4 == 0 ); // only allow for word-sized blocks
    Tanto_V_MemBlock*  block = NULL;
    struct BlockChain* chain = NULL;
    switch (memType) 
    {
        case TANTO_V_MEMORY_HOST_GRAPHICS_TYPE: chain = &blockChainHostGraphics; break;
        case TANTO_V_MEMORY_HOST_TRANSFER_TYPE: chain = &blockChainHostTransfer; break;
        case TANTO_V_MEMORY_DEVICE_TYPE:        chain = &blockChainDeviceGraphics; break;
        default: block = NULL; assert(0);
    }

    if (0 == alignment)
        alignment = chain->defaultAlignment;

    block = requestBlock(size, alignment, chain); 
    Tanto_V_BufferRegion region;
    region.offset = block->offset;
    region.memBlockId = block->id;
    region.size = block->size;
    region.buffer = chain->buffer;
    region.hostData = chain->hostData + block->offset;
    region.pChain = chain;

    return region;
}

Tanto_V_BufferRegion tanto_v_RequestBufferRegion(const size_t size, 
        const VkBufferUsageFlags flags, const Tanto_V_MemoryType memType)
{
    uint32_t alignment = 0;
    // the order of this if statements matters. it garuantees the maximum
    // alignment is chose if multiple flags are present
    if ( VK_BUFFER_USAGE_STORAGE_BUFFER_BIT & flags)
    {
        alignment = deviceProperties.limits.minStorageBufferOffsetAlignment;
        // must satisfy alignment requirements for storage buffers
    }
    if ( VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT & flags && 
            alignment < deviceProperties.limits.minUniformBufferOffsetAlignment)
    {
        // must satisfy alignment requirements for uniform buffers
        alignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
    }
    return tanto_v_RequestBufferRegionAligned(size, alignment, memType);
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
        const VkImageAspectFlags aspectMask,
        const VkSampleCountFlags sampleCount)
{
    assert( width * height < MEMORY_SIZE_DEV_IMAGE );

    assert(deviceProperties.limits.framebufferColorSampleCounts >= sampleCount);
    assert(deviceProperties.limits.framebufferDepthSampleCounts >= sampleCount);

    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = sampleCount,
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

    V1_PRINT("Requesting image of size %ld (0x%lx) \n", memReqs.size, memReqs.size);

    const Tanto_V_MemBlock* block = requestBlock(memReqs.size, memReqs.alignment, &blockChainDeviceImage);
    image.memBlockId = block->id;
    image.size = memReqs.size;
    image.extent.depth = 1;
    image.extent.width = width;
    image.extent.height = height;

    vkBindImageMemory(device, image.handle, blockChainDeviceImage.memory, block->offset);

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
    if (image.sampler != VK_NULL_HANDLE)
        vkDestroySampler(device, image.sampler, NULL);
    vkDestroyImageView(device, image.view, NULL);
    vkDestroyImage(device, image.handle, NULL);
    freeBlock(&blockChainDeviceImage, image.memBlockId);
}

void tanto_v_FreeBufferRegion(Tanto_V_BufferRegion* pRegion)
{
    freeBlock(pRegion->pChain, pRegion->memBlockId);
    memset(pRegion->hostData, 0, pRegion->size);
    memset(pRegion, 0, sizeof(Tanto_V_BufferRegion));
}

void tanto_v_TransferToDevice(Tanto_V_BufferRegion* pRegion)
{
    const Tanto_V_BufferRegion srcRegion = *pRegion;
    assert( srcRegion.pChain == &blockChainHostGraphics ); // only chain it makes sense to transfer from
    Tanto_V_BufferRegion destRegion = tanto_v_RequestBufferRegion(srcRegion.size, 0, TANTO_V_MEMORY_DEVICE_TYPE);

    Tanto_V_CommandPool cmdPool = tanto_v_RequestOneTimeUseCommand(); // arbitrary index;

    VkBufferCopy copy;
    copy.srcOffset = srcRegion.offset;
    copy.dstOffset = destRegion.offset;
    copy.size      = srcRegion.size;

    vkCmdCopyBuffer(cmdPool.buffer, srcRegion.buffer, destRegion.buffer, 1, &copy);

    tanto_v_SubmitOneTimeCommandAndWait(&cmdPool, 0);

    tanto_v_FreeBufferRegion(pRegion);

    *pRegion = destRegion;
}

void tanto_v_CleanUpMemory()
{
    freeBlockChain(&blockChainHostGraphics);
    freeBlockChain(&blockChainDeviceImage);
    freeBlockChain(&blockChainHostTransfer);
};
