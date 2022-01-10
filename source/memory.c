#include "memory.h"
#include "common.h"
#include "dtags.h"
#include "command.h"
#include "private.h"
#include "video.h"
#include <assert.h>
#include <hell/common.h>
#include <hell/debug.h>
#include <hell/minmax.h>
#include <stdlib.h>
#include <string.h>

// HVC = Host Visible and Coherent
// DL = Device Local
//#define MAX_BLOCKS 100000 we overflowed the stack...

#define MB 0x100000

typedef Obdn_BufferRegion BufferRegion;
//
// static uint32_t findMemoryType(uint32_t memoryTypeBitsRequirement)
// __attribute__ ((unused));
#if UNIX
static void printBufferMemoryReqs(const VkMemoryRequirements* reqs)
    __attribute__((unused));
static void printBlockChainInfo(const BlockChain* chain)
    __attribute__((unused));
#endif

#define DPRINT(fmt, ...) hell_DebugPrint(OBDN_DEBUG_TAG_MEM, fmt, ##__VA_ARGS__)

static void
printBufferMemoryReqs(const VkMemoryRequirements* reqs)
{
    DPRINT("Size: %zu\tAlignment: %zu\n", reqs->size, reqs->alignment);
}

static void
printBlockChainInfo(const BlockChain* chain)
{
    DPRINT("BlockChain %s:\n", chain->name);
    DPRINT("totalSize: %zu\t count: %d\t cur: %d\t nextBlockId: %d\n",
           chain->totalSize, chain->count, chain->cur, chain->nextBlockId);
    DPRINT("memory: %p\t buffer: %p\t hostData: %p\n", chain->memory,
           chain->buffer, chain->hostData);
    DPRINT("Blocks: \n");
    for (int i = 0; i < chain->count; i++)
    {
        const Obdn_MemBlock* block = &chain->blocks[i];
        DPRINT("{ Block %d: size = %zu, offset = %zu, inUse = %s, id: %d}, ", i,
               block->size, block->offset, block->inUse ? "true" : "false",
               block->id);
    }
    DPRINT("\n");
}

static uint32_t
findMemoryType(const Obdn_Memory* memory, uint32_t memoryTypeBitsRequirement)
{
    const uint32_t memoryCount = memory->properties.memoryTypeCount;
    for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex)
    {
        const uint32_t memoryTypeBits = (1 << memoryIndex);
        if (memoryTypeBits & memoryTypeBitsRequirement)
            return memoryIndex;
    }
    return -1;
}

static uint32_t
alignmentForBufferUsage(const Obdn_Memory* memory, VkBufferUsageFlags flags)
{
    uint32_t alignment = 16; // arbitrary default alignment for all allocations. should test lower values.
    if (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT & flags)
        alignment = MAX(
            memory->deviceProperties->limits.minStorageBufferOffsetAlignment,
            alignment);
    if (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT & flags)
        alignment = MAX(
            memory->deviceProperties->limits.minUniformBufferOffsetAlignment,
            alignment);
    if (VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR & flags)
        alignment =
            MAX(256,
                alignment); // 256 comes from spec for
                            // VkAccelerationStructureCreateInfoKHR - see offset
    return alignment;
}

static void
initBlockChain(Obdn_Memory* memory, const Obdn_MemoryType memType,
               const VkDeviceSize memorySize, const uint32_t memTypeIndex,
               const VkBufferUsageFlags bufferUsageFlags, const bool mapBuffer,
               const char* name, struct BlockChain* chain)
{
    memset(chain, 0, sizeof(BlockChain));
    strcpy(chain->name, name);
    if (memorySize == 0)
        return; // basically saying we arent using this memory type
    if (memorySize % 0x40 != 0)
        hell_Error(HELL_ERR_FATAL,
                   "Failed to initialize %s block chain because requested %zu "
                   "bytes is not divisible by 0x40\n",
                   name, memorySize);
    assert(memorySize % 0x40 ==
           0); // make sure memorysize is 64 byte aligned (arbitrary choice)
    chain->memory           = memory;
    chain->count            = 1;
    chain->cur              = 0;
    chain->totalSize        = memorySize;
    chain->usedSize         = 0;
    chain->nextBlockId      = 1;
    chain->defaultAlignment = 4;
    chain->bufferFlags      = bufferUsageFlags;
    chain->blocks[0].inUse  = false;
    chain->blocks[0].offset = 0;
    chain->blocks[0].size   = memorySize;
    chain->blocks[0].id     = 0;
    assert(strlen(name) < 16);
    strcpy(chain->name, name);

    if (chain->totalSize == 0)
        return; // nothing else to do

    VkExportMemoryAllocateInfo exportMemoryAllocInfo;
    const void*                pNext = NULL;
    if (memType == OBDN_MEMORY_EXTERNAL_DEVICE_TYPE)
    {
        exportMemoryAllocInfo.sType =
            VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
            #ifdef WIN32
        exportMemoryAllocInfo.handleTypes =
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
            #else
        exportMemoryAllocInfo.handleTypes =
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
            #endif
        exportMemoryAllocInfo.pNext = NULL;
        pNext                       = &exportMemoryAllocInfo;
    }

    const VkMemoryAllocateFlagsInfo allocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .pNext = pNext};

    const VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = &allocFlagsInfo,
        .allocationSize  = memorySize,
        .memoryTypeIndex = memTypeIndex,
    };

    V_ASSERT(
        vkAllocateMemory(memory->instance->device, &allocInfo, NULL, &chain->vkmemory));

    if (bufferUsageFlags)
    {

        uint32_t queueFamilyIndex;
        switch (memType)
        {
        case OBDN_MEMORY_HOST_GRAPHICS_TYPE:
            queueFamilyIndex = memory->instance->graphicsQueueFamily.index;
            break;
        case OBDN_MEMORY_HOST_TRANSFER_TYPE:
            queueFamilyIndex = memory->instance->transferQueueFamily.index;
            break;
        case OBDN_MEMORY_DEVICE_TYPE:
            queueFamilyIndex = memory->instance->graphicsQueueFamily.index;
            break;
        case OBDN_MEMORY_EXTERNAL_DEVICE_TYPE:
            queueFamilyIndex = memory->instance->graphicsQueueFamily.index;
            break;
        default:
            assert(0);
            break;
        }

        VkBufferCreateInfo ci = {
            .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .usage                 = bufferUsageFlags,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices   = &queueFamilyIndex,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // queue family determined
                                                      // by first use
            // TODO this sharing mode is why we need to expand Obdn_V_MemoryType
            // to specify queue usage as well. So we do need graphics type,
            // transfer type, and compute types
            .size        = memorySize};

        V_ASSERT(vkCreateBuffer(memory->instance->device, &ci, NULL, &chain->buffer));

        V_ASSERT(vkBindBufferMemory(memory->instance->device, chain->buffer,
                                    chain->vkmemory, 0));

        VkMemoryRequirements memReqs;

        vkGetBufferMemoryRequirements(memory->instance->device, chain->buffer, &memReqs);

        // chain->defaultAlignment = memReqs.alignment;

        DPRINT("Host Buffer ALIGNMENT: %zu\n", memReqs.alignment);

        const VkBufferDeviceAddressInfo addrInfo = {
            .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = chain->buffer};

        chain->bufferAddress =
            vkGetBufferDeviceAddress(memory->instance->device, &addrInfo);

        if (mapBuffer)
            V_ASSERT(vkMapMemory(memory->instance->device, chain->vkmemory, 0,
                                 VK_WHOLE_SIZE, 0, (void**)&chain->hostData));
        else
            chain->hostData = NULL;
    }
    else
    {
        chain->buffer   = VK_NULL_HANDLE;
        chain->hostData = NULL;
    }
}

static void
freeBlockChain(Obdn_Memory* memory, struct BlockChain* chain)
{
    memset(chain->blocks, 0, MAX_BLOCKS * sizeof(Obdn_MemBlock));
    if (chain->buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(memory->instance->device, chain->buffer, NULL);
        if (chain->hostData != NULL)
            vkUnmapMemory(memory->instance->device, chain->vkmemory);
        chain->buffer = VK_NULL_HANDLE;
    }
    vkFreeMemory(memory->instance->device, chain->vkmemory, NULL);
    memset(chain, 0, sizeof(*chain));
}

typedef struct AvailabilityInfo {
    bool           foundBlock;
    u32            index;
    u64            newOffset;
} AvailabilityInfo;

static AvailabilityInfo
findAvailableBlockIndex(struct BlockChain* chain, VkDeviceSize size, VkDeviceSize alignment)
{
    const int count = chain->count;
    DPRINT(">>> requesting block of size %d from chain %s with totalSize %zu\n",
           size, chain->name, chain->totalSize);
    assert(size < chain->totalSize);
    assert(count > 0);
    assert(alignment != 0);
    for (u32 i = 0; i < count; i++)
    {
        const Obdn_MemBlock* block = &chain->blocks[i];
        if (block->inUse || block->size < size) continue;
        // nextPossibleOffset may be the same as or greater than block->offset.
        u64 nextPossibleOffset = hell_Align(block->offset, alignment);
        u64 blockEnd = block->offset + block->size;
        if (nextPossibleOffset > blockEnd) continue;
        if (blockEnd - nextPossibleOffset < size) continue;
        return (AvailabilityInfo){true, i, nextPossibleOffset};
        // block's new size will be greater than or equal to the resource size;
    }
    return (AvailabilityInfo){false};
}

// from i = firstIndex to i = lastIndex - 1, swap block i with block i + 1
// note this can operate on indices beyond chain->count
static void
rotateBlockUp(const size_t fromIndex, const size_t toIndex,
              struct BlockChain* chain)
{
    assert(fromIndex >= 0);
    assert(toIndex < MAX_BLOCKS);
    assert(toIndex > fromIndex);
    for (int i = fromIndex; i < toIndex; i++)
    {
        Obdn_MemBlock temp = chain->blocks[i];
        chain->blocks[i]     = chain->blocks[i + 1];
        chain->blocks[i + 1] = temp;
    }
}

static void
rotateBlockDown(const size_t fromIndex, const size_t toIndex,
                struct BlockChain* chain)
{
    assert(toIndex >= 0);
    assert(fromIndex < MAX_BLOCKS);
    assert(fromIndex > toIndex);
    for (int i = fromIndex; i > toIndex; i--)
    {
        Obdn_MemBlock temp = chain->blocks[i];
        chain->blocks[i]     = chain->blocks[i - 1];
        chain->blocks[i - 1] = temp;
        assert(chain->blocks[i - 1].size < chain->totalSize);
    }
}

static void
mergeBlocks(struct BlockChain* chain)
{
    assert(chain->count > 1); // must have at least 2 blocks to defragment
    for (int i = 0; i < chain->count - 1; i++)
    {
        Obdn_MemBlock* curr = &chain->blocks[i];
        Obdn_MemBlock* next = &chain->blocks[i + 1];
        if (!curr->inUse && !next->inUse)
        {
            // combine them together
            const VkDeviceSize cSize = curr->size + next->size;
            // we could make the other id available for re-use as well
            curr->size               = cSize;
            // set next block to 0
            memset(next, 0, sizeof(Obdn_MemBlock));
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

static void
defragment(struct BlockChain* chain)
{
    // TODO: implement a proper defragment function
    DPRINT("Empty defragment function called\n");
}

static bool
chainIsOrdered(const struct BlockChain* chain)
{
    for (int i = 0; i < chain->count - 1; i++)
    {
        const Obdn_MemBlock* curr = &chain->blocks[i];
        const Obdn_MemBlock* next = &chain->blocks[i + 1];
        if (curr->offset > next->offset)
            return false;
    }
    return true;
}

static Obdn_MemBlock*
requestBlock(const u64 size, const u64 alignment,
             struct BlockChain* chain)
{
    AvailabilityInfo availInfo = findAvailableBlockIndex(chain, size, alignment);
    if (!availInfo.foundBlock) // try defragmenting. if that fails we're done.
    {
        defragment(chain);
        availInfo = findAvailableBlockIndex(chain, size, alignment);
        if (!availInfo.foundBlock)
        {
            DPRINT("Memory allocation failed\n");
            exit(0);
        }
    }
    Obdn_MemBlock* curBlock = &chain->blocks[availInfo.index];
    if (curBlock->size == size &&
        (curBlock->offset % alignment == 0)) // just reuse this block;
    {
        curBlock->inUse = true;
        chain->usedSize += curBlock->size;
        DPRINT(">> Re-using block %d of size %09zu from chain %s. %zu bytes "
               "out of %zu now in use.\n",
               curBlock->id, curBlock->size, chain->name, chain->usedSize,
               chain->totalSize);
        return curBlock;
    }
    // split the block
    const size_t     newBlockIndex = chain->count++;
    Obdn_MemBlock*   newBlock = &chain->blocks[newBlockIndex];
    assert(newBlockIndex < MAX_BLOCKS);
    assert(newBlock->inUse == false);
    // we will assume correct alignment for the first block... TODO Make sure this is enforced somehow.
    // if the newOffset is not equal to currentOffset, we must change the currentOffset 
    // to the new offset, alter the size, and also add the difference in offsets to the
    // size of the previous block. This is why we check if the index is 0 and hope for the best in that case.
    // we have already checked to make sure that the block has enough space to fit the resource given its new offset. 
    u64 diff = 0;
    if (availInfo.index > 0 && availInfo.newOffset != curBlock->offset) 
    {
        diff   = availInfo.newOffset - curBlock->offset;
        curBlock->size = curBlock->size - diff;
        curBlock->offset = availInfo.newOffset;
        chain->blocks[availInfo.index - 1].size = chain->blocks[availInfo.index - 1].size + diff;
    }
    newBlock->size                   = curBlock->size   - size;
    newBlock->offset                 = curBlock->offset + size;
    newBlock->id                     = chain->nextBlockId++;
    curBlock->size                   = size;
    curBlock->inUse                  = true;
    if (newBlockIndex != availInfo.index + 1)
        rotateBlockDown(newBlockIndex, availInfo.index + 1, chain);
    assert(curBlock->size < chain->totalSize);
    assert(chainIsOrdered(chain));
    chain->usedSize += curBlock->size + diff; //need to factor in the diff if we had to move the block's offset forward.
    DPRINT(">> Alocating block %d of size %09zu from chain %s. %zu bytes out "
           "of %zu now in use.\n",
           curBlock->id, curBlock->size, chain->name, chain->usedSize,
           chain->totalSize);
    return curBlock;
}

static void
freeBlock(struct BlockChain* chain, const uint32_t id)
{
    assert(id < chain->nextBlockId);
    const size_t blockCount = chain->count;
    int          blockIndex = 0;
    for (; blockIndex < blockCount; blockIndex++)
    {
        if (chain->blocks[blockIndex].id == id)
            break;
    }
    assert(blockIndex < blockCount); // block must not have come from this chain
    Obdn_MemBlock*   block = &chain->blocks[blockIndex];
    const VkDeviceSize size  = block->size;
    block->inUse             = false;
    mergeBlocks(chain);
    chain->usedSize -= size;
    DPRINT(">> Freeing block %d of size %09zu from chain %s. %zu bytes out of "
           "%zu now in use.\n",
           id, size, chain->name, chain->usedSize, chain->totalSize);
}

void
obdn_CreateMemory(const Obdn_Instance* instance, const uint32_t hostGraphicsBufferMB,
                  const uint32_t deviceGraphicsBufferMB,
                  const uint32_t deviceGraphicsImageMB, const uint32_t hostTransferBufferMB,
                  const uint32_t deviceExternalGraphicsImageMB, Obdn_Memory* memory)
{
    assert(hostGraphicsBufferMB < 10000); // maximum 10 GB
    assert(deviceGraphicsBufferMB < 10000); // maximum 10 GB
    assert(deviceGraphicsImageMB < 10000); // maximum 10 GB
    assert(hostTransferBufferMB < 10000); // maximum 10 GB
    assert(deviceExternalGraphicsImageMB < 10000); // maximum 10 GB
    memset(memory, 0, sizeof(Obdn_Memory));
    memory->instance                 = instance;

    memory->deviceProperties = obdn_GetPhysicalDeviceProperties(instance);

    vkGetPhysicalDeviceMemoryProperties(instance->physicalDevice,
                                        &memory->properties);

    DPRINT("Memory Heap Info:\n");
    for (int i = 0; i < memory->properties.memoryHeapCount; i++)
    {
        const char* typestr = memory->properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "Device" : "Host";
        //TODO This DPRINT causes a crash on windows... just debug info so not important but worth investigating at some point
        //DPRINT("Heap %d: Size %zu: %s local\n", i, memory->properties.memoryHeaps[i].size, typestr);
        // note there are other possible flags, but seem to only deal with
        // multiple gpus
    }

    bool foundHvc = false;
    bool foundDl  = false;

    DPRINT("Memory Type Info:\n");
    for (int i = 0; i < memory->properties.memoryTypeCount; i++)
    {
        VkMemoryPropertyFlags flags =
            memory->properties.memoryTypes[i].propertyFlags;
        DPRINT("Type %d: Heap Index: %d Flags: | %s%s%s%s%s%s\n", i,
               memory->properties.memoryTypes[i].heapIndex,
               flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? "Device Local | "
                                                           : "",
               flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? "Host Visible | "
                                                           : "",
               flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? "Host Coherent | "
                                                            : "",
               flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? "Host Cached | "
                                                          : "",
               flags & VK_MEMORY_PROPERTY_PROTECTED_BIT ? "Protected | " : "",
               flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
                   ? "Lazily allocated | "
                   : "");
        if ((flags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) &&
            !foundHvc)
        {
            memory->hostVisibleCoherentTypeIndex = i;
            foundHvc                             = true;
        }
        if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !foundDl)
        {
            memory->deviceLocalTypeIndex = i;
            foundDl                      = true;
        }
    }

    assert(foundHvc);
    assert(foundDl);
    DPRINT("Host Visible and Coherent memory type index found: %d\n",
           memory->hostVisibleCoherentTypeIndex);
    DPRINT("Device local memory type index found: %d\n",
           memory->deviceLocalTypeIndex);

    VkBufferUsageFlags hostGraphicsFlags =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBufferUsageFlags hostTransferFlags =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBufferUsageFlags devBufFlags =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    DPRINT("Obsidian: Initializing memory block chains...\n");
    initBlockChain(memory, OBDN_MEMORY_HOST_GRAPHICS_TYPE,
                   hostGraphicsBufferMB * MB,
                   memory->hostVisibleCoherentTypeIndex, hostGraphicsFlags,
                   true, "hstGraphBuffer",
                   &memory->blockChainHostGraphicsBuffer);
    initBlockChain(memory, OBDN_MEMORY_HOST_TRANSFER_TYPE,
                   hostTransferBufferMB * MB,
                   memory->hostVisibleCoherentTypeIndex, hostTransferFlags,
                   true, "hstTransBuffer",
                   &memory->blockChainHostTransferBuffer);
    initBlockChain(memory, OBDN_MEMORY_DEVICE_TYPE,
                   deviceGraphicsBufferMB * MB,
                   memory->deviceLocalTypeIndex, devBufFlags, false,
                   "devBuffer", &memory->blockChainDeviceGraphicsBuffer);
    initBlockChain(memory, OBDN_MEMORY_DEVICE_TYPE,
                   deviceGraphicsImageMB * MB,
                   memory->deviceLocalTypeIndex, 0, false, "devImage",
                   &memory->blockChainDeviceGraphicsImage);
    initBlockChain(memory, OBDN_MEMORY_EXTERNAL_DEVICE_TYPE,
                   deviceExternalGraphicsImageMB * MB,
                   memory->deviceLocalTypeIndex, 0, false, "devExtImage",
                   &memory->blockChainExternalDeviceGraphicsImage);
}

Obdn_BufferRegion
obdn_RequestBufferRegionAligned(Obdn_Memory* memory, const size_t size,
                                uint32_t                alignment,
                                const Obdn_MemoryType memType)
{
    assert(size > 0);
    if (size % 4 != 0) // only allow for word-sized blocks
    {
        hell_Error(HELL_ERR_FATAL, "Size %zu is not 4 byte aligned.", size);
    }
    Obdn_MemBlock*   block = NULL;
    struct BlockChain* chain = NULL;
    switch (memType)
    {
    case OBDN_MEMORY_HOST_GRAPHICS_TYPE:
        chain = &memory->blockChainHostGraphicsBuffer;
        break;
    case OBDN_MEMORY_HOST_TRANSFER_TYPE:
        chain = &memory->blockChainHostTransferBuffer;
        break;
    case OBDN_MEMORY_DEVICE_TYPE:
        chain = &memory->blockChainDeviceGraphicsBuffer;
        break;
    default:
        block = NULL;
        assert(0);
    }

    if (0 == alignment)
        alignment = chain->defaultAlignment;
    block = requestBlock(size, alignment, chain);

    Obdn_BufferRegion region = {0};
    region.offset              = block->offset;
    region.memBlockId          = block->id;
    region.size                = size;
    region.buffer              = chain->buffer;
    region.pChain              = chain;

    // TODO this check is very brittle. we should probably change V_MemoryType
    // to a mask with flags for the different requirements. One bit for host or
    // device, one bit for transfer capabale, one bit for external, one bit for
    // image or buffer
    if (memType == OBDN_MEMORY_HOST_TRANSFER_TYPE ||
        memType == OBDN_MEMORY_HOST_GRAPHICS_TYPE)
        region.hostData = chain->hostData + block->offset;
    else
        region.hostData = NULL;

    return region;
}

Obdn_BufferRegion
obdn_RequestBufferRegion(Obdn_Memory* memory, const size_t size,
                         const VkBufferUsageFlags flags,
                         const Obdn_MemoryType  memType)
{
    uint32_t alignment = alignmentForBufferUsage(memory, flags);
    return obdn_RequestBufferRegionAligned(memory, 
        size, alignment,
        memType); // TODO: fix this. find the maximum alignment and choose that
}

Obdn_BufferRegion
obdn_RequestBufferRegionArray(Obdn_Memory* memory, uint32_t elemSize, uint32_t elemCount,
                              VkBufferUsageFlags flags,
                              Obdn_MemoryType  memType)
{
    // calculate the size needed and call requestBufferRegionAligned.
    // make sure to set the stride.
    uint32_t alignment = alignmentForBufferUsage(memory, flags);
    uint32_t stride    = hell_Align(elemSize, alignment);
    uint64_t size      = stride * elemCount;
    Obdn_BufferRegion region = obdn_RequestBufferRegionAligned(memory, size, alignment, memType);
    region.stride = stride;
    return region;
}

uint32_t
obdn_GetMemoryType(const Obdn_Memory* memory, uint32_t typeBits,
                   const VkMemoryPropertyFlags properties)
{
    for (uint32_t i = 0; i < memory->properties.memoryTypeCount; i++)
    {
        if (((typeBits & (1 << i)) > 0) &&
            (memory->properties.memoryTypes[i].propertyFlags & properties) ==
                properties)
        {
            return i;
        }
    }
    assert(0);
    return ~0u;
}

Obdn_Image
obdn_CreateImage(Obdn_Memory* memory, const uint32_t width,
                 const uint32_t height, const VkFormat format,
                 const VkImageUsageFlags  usageFlags,
                 const VkImageAspectFlags aspectMask,
                 const VkSampleCountFlags sampleCount, const uint32_t mipLevels,
                 const Obdn_MemoryType memType)
{
    assert(mipLevels > 0);
    assert(memType == OBDN_MEMORY_DEVICE_TYPE ||
           memType == OBDN_MEMORY_EXTERNAL_DEVICE_TYPE);

    assert(memory->deviceProperties->limits.framebufferColorSampleCounts >=
           sampleCount);
    assert(memory->deviceProperties->limits.framebufferDepthSampleCounts >=
           sampleCount);

    void* pNext = NULL;

    VkExternalMemoryImageCreateInfo externalImageInfo;
    if (memType == OBDN_MEMORY_EXTERNAL_DEVICE_TYPE)
    {
        externalImageInfo.sType =
            VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
        externalImageInfo.handleTypes =
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        externalImageInfo.pNext = NULL;
        pNext                   = &externalImageInfo;
    }

    uint32_t          queueFamilyIndex = memory->instance->graphicsQueueFamily.index;
    VkImageCreateInfo imageInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                   .pNext = pNext,
                                   .imageType   = VK_IMAGE_TYPE_2D,
                                   .format      = format,
                                   .extent      = {width, height, 1},
                                   .mipLevels   = mipLevels,
                                   .arrayLayers = 1,
                                   .samples     = sampleCount,
                                   .tiling      = VK_IMAGE_TILING_OPTIMAL,
                                   .usage       = usageFlags,
                                   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                   .queueFamilyIndexCount = 1,
                                   .pQueueFamilyIndices   = &queueFamilyIndex,
                                   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    Obdn_Image image = {0};

    V_ASSERT(vkCreateImage(memory->instance->device, &imageInfo, NULL, &image.handle));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(memory->instance->device, image.handle, &memReqs);

    DPRINT("Requesting image of size %zu (0x%zx) \n", memReqs.size,
           memReqs.size);
    DPRINT("Width * Height * Format size = %d\n", width * height * 4);
    DPRINT("Required memory bits for image: %0x\n", memReqs.memoryTypeBits);

    image.size          = memReqs.size;
    image.extent.depth  = 1;
    image.extent.width  = width;
    image.extent.height = height;
    image.mipLevels     = mipLevels;
    image.aspectMask    = aspectMask;
    image.format        = format;
    image.usageFlags    = usageFlags;

    switch (memType)
    {
    case OBDN_MEMORY_DEVICE_TYPE:
        image.pChain = &memory->blockChainDeviceGraphicsImage;
        break;
    case OBDN_MEMORY_EXTERNAL_DEVICE_TYPE:
        image.pChain = &memory->blockChainExternalDeviceGraphicsImage;
        break;
    default:
        assert(0);
    }

    const Obdn_MemBlock* block =
        requestBlock(memReqs.size, memReqs.alignment, image.pChain);
    image.memBlockId = block->id;
    image.offset     = block->offset;

    vkBindImageMemory(memory->instance->device, image.handle, image.pChain->vkmemory,
                      block->offset);

    VkImageViewCreateInfo viewInfo = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = image.handle,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .components       = {0, 0, 0, 0}, // no swizzling
        .format           = format,
        .subresourceRange = {.aspectMask     = aspectMask,
                             .baseMipLevel   = 0,
                             .levelCount     = mipLevels,
                             .baseArrayLayer = 0,
                             .layerCount     = 1}};

    V_ASSERT(vkCreateImageView(memory->instance->device, &viewInfo, NULL, &image.view));

    image.sampler = VK_NULL_HANDLE;
    image.layout  = imageInfo.initialLayout;

    return image;
}

void
obdn_FreeImage(Obdn_Image* image)
{
    assert(image->size != 0);
    if (image->sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(image->pChain->memory->instance->device, image->sampler, NULL);
    }
    vkDestroyImageView(image->pChain->memory->instance->device, image->view, NULL);
    vkDestroyImage(image->pChain->memory->instance->device, image->handle, NULL);
    freeBlock(image->pChain, image->memBlockId);
    memset(image, 0, sizeof(Obdn_Image));
}

void
obdn_FreeBufferRegion(Obdn_BufferRegion* pRegion)
{
    assert(pRegion->size != 0);
    freeBlock(pRegion->pChain, pRegion->memBlockId);
    if (pRegion->hostData)
        memset(pRegion->hostData, 0, pRegion->size);
    memset(pRegion, 0, sizeof(Obdn_BufferRegion));
}

void
obdn_CopyBufferRegion(const Obdn_BufferRegion* src,
                        Obdn_BufferRegion*       dst)
{
    Obdn_Command cmd =
        obdn_CreateCommand(src->pChain->memory->instance, OBDN_V_QUEUE_GRAPHICS_TYPE); // arbitrary index;

    obdn_BeginCommandBuffer(cmd.buffer);

    VkBufferCopy copy;
    copy.srcOffset = src->offset;
    copy.dstOffset = dst->offset;
    copy.size      = src->size;

    vkCmdCopyBuffer(cmd.buffer, src->buffer, dst->buffer, 1, &copy);

    obdn_EndCommandBuffer(cmd.buffer);

    obdn_SubmitAndWait(&cmd, 0);

    obdn_DestroyCommand(cmd);
}

void
obdn_v_CopyImageToBufferRegion(const Obdn_Image*  image,
                               Obdn_BufferRegion* bufferRegion)
{
    // TODO
}

void
obdn_TransferToDevice(Obdn_Memory* memory, Obdn_BufferRegion* pRegion)
{
    const Obdn_BufferRegion srcRegion = *pRegion;
    assert(srcRegion.pChain ==
           &memory->blockChainHostGraphicsBuffer); // only chain it makes sense
                                                   // to transfer from
    Obdn_BufferRegion destRegion = obdn_RequestBufferRegion(memory, 
        srcRegion.size, 0, OBDN_MEMORY_DEVICE_TYPE);

    obdn_CopyBufferRegion(&srcRegion, &destRegion);

    obdn_FreeBufferRegion(pRegion);

    *pRegion = destRegion;
}

void
obdn_DestroyMemory(Obdn_Memory* memory)
{
    freeBlockChain(memory, &memory->blockChainHostGraphicsBuffer);
    freeBlockChain(memory, &memory->blockChainHostTransferBuffer);
    freeBlockChain(memory, &memory->blockChainDeviceGraphicsImage);
    freeBlockChain(memory, &memory->blockChainDeviceGraphicsBuffer);
    freeBlockChain(memory, &memory->blockChainExternalDeviceGraphicsImage);
}

VkDeviceAddress
obdn_GetBufferRegionAddress(const BufferRegion* region)
{
    assert(region->pChain->bufferAddress != 0);
    return region->pChain->bufferAddress + region->offset;
}

void
obdn_CreateUnmanagedBuffer(Obdn_Memory*             memory,
                           const VkBufferUsageFlags bufferUsageFlags,
                           const uint32_t           memorySize,
                           const Obdn_MemoryType  type,
                           VkDeviceMemory* pMemory, VkBuffer* pBuffer)
{
    uint32_t typeIndex;
    switch (type)
    {
    case OBDN_MEMORY_HOST_GRAPHICS_TYPE:
        typeIndex = memory->hostVisibleCoherentTypeIndex;
        break;
    case OBDN_MEMORY_HOST_TRANSFER_TYPE:
        typeIndex = memory->hostVisibleCoherentTypeIndex;
        break;
    case OBDN_MEMORY_DEVICE_TYPE:
        typeIndex = memory->deviceLocalTypeIndex;
        break;
    case OBDN_MEMORY_EXTERNAL_DEVICE_TYPE:
        typeIndex = memory->deviceLocalTypeIndex;
        break;
    }

    const VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memorySize,
        .memoryTypeIndex = typeIndex};

    VkBufferCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .usage = bufferUsageFlags,
        .sharingMode =
            VK_SHARING_MODE_EXCLUSIVE, // queue determined by first use
        .size = memorySize};

    V_ASSERT(vkAllocateMemory(memory->instance->device, &allocInfo, NULL, pMemory));

    V_ASSERT(vkCreateBuffer(memory->instance->device, &ci, NULL, pBuffer));

    V_ASSERT(vkBindBufferMemory(memory->instance->device, *pBuffer, *pMemory, 0));
}

const VkDeviceMemory
obdn_GetDeviceMemory(const Obdn_Memory* memory, const Obdn_MemoryType memType)
{
    switch (memType)
    {
    case OBDN_MEMORY_EXTERNAL_DEVICE_TYPE:
        return memory->blockChainExternalDeviceGraphicsImage.vkmemory;
    default:
        assert(0); // TODO
        return 0;
    }
}

const VkDeviceSize
obdn_GetMemorySize(const Obdn_Memory* memory, const Obdn_MemoryType memType)
{
    switch (memType)
    {
    case OBDN_MEMORY_EXTERNAL_DEVICE_TYPE:
        return memory->blockChainExternalDeviceGraphicsImage.totalSize;
    default:
        assert(0); // TODO
        return 0;
    }
}

#ifdef WIN32
bool obdn_GetExternalMemoryWin32Handle(const Obdn_Memory* memory, HANDLE* handle, uint64_t* size)
{
    VkMemoryGetWin32HandleInfoKHR handleInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
        .memory = obdn_GetDeviceMemory(memory, OBDN_MEMORY_EXTERNAL_DEVICE_TYPE),
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
    };

    V_ASSERT( vkGetMemoryWin32HandleKHR(obdn_GetDevice(memory->instance), &handleInfo, handle) );

    *size = obdn_GetMemorySize(memory, OBDN_MEMORY_EXTERNAL_DEVICE_TYPE);

    assert(*size);
    return true;
}
#else
bool obdn_GetExternalMemoryFd(const Obdn_Memory* memory, int* fd, uint64_t* size)
{
    // fast path
    VkMemoryGetFdInfoKHR fdInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
        .memory = obdn_GetDeviceMemory(memory, OBDN_MEMORY_EXTERNAL_DEVICE_TYPE),
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
    };

    V_ASSERT( vkGetMemoryFdKHR(obdn_GetDevice(memory->instance), &fdInfo, fd) );

    *size = obdn_GetMemorySize(memory, OBDN_MEMORY_EXTERNAL_DEVICE_TYPE);

    assert(*size);
    return true;
}
#endif

uint64_t
obdn_SizeOfMemory(void)
{
    return sizeof(Obdn_Memory);
}

Obdn_Memory* obdn_AllocMemory(void)
{
    return hell_Malloc(sizeof(Obdn_Memory));
}

static void 
simpleBlockchainReport(const BlockChain* chain)
{
    float percent = (float)chain->usedSize / chain->totalSize;
    hell_Print("Blockchain: %s Used Size: %d Total Size: %d Percent Used: %f\n", chain->name, chain->usedSize, chain->totalSize, percent);
}

void 
obdn_MemoryReportSimple(const Obdn_Memory* memory)
{
    hell_Print("Memory Report\n");
    simpleBlockchainReport(&memory->blockChainHostGraphicsBuffer);
    simpleBlockchainReport(&memory->blockChainDeviceGraphicsBuffer);
    simpleBlockchainReport(&memory->blockChainDeviceGraphicsImage);
    simpleBlockchainReport(&memory->blockChainHostTransferBuffer);
    simpleBlockchainReport(&memory->blockChainExternalDeviceGraphicsImage);
}

void 
obdn_GetImageMemoryUsage(const Obdn_Memory* memory, uint64_t* bytes_in_use, uint64_t* total_bytes)
{
    *bytes_in_use = memory->blockChainDeviceGraphicsImage.usedSize;
    *total_bytes = memory->blockChainDeviceGraphicsImage.totalSize;
}

const Obdn_Instance* obdn_GetMemoryInstance(const Obdn_Memory* memory)
{
    return memory->instance;
}
