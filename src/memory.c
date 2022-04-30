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

typedef Onyx_Memory Memory;
typedef Onyx_BufferRegion BufferRegion;
typedef Onyx_MemBlock Block;
//
// static uint32_t findMemoryType(uint32_t memoryTypeBitsRequirement)
// __attribute__ ((unused));
#if UNIX
static void printBufferMemoryReqs(const VkMemoryRequirements* reqs)
    __attribute__((unused));
static void printBlockChainInfo(const BlockChain* chain)
    __attribute__((unused));
#endif

#define DPRINT(fmt, ...) hell_DebugPrint(ONYX_DEBUG_TAG_MEM, fmt, ##__VA_ARGS__)

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
        const Onyx_MemBlock* block = &chain->blocks[i];
        DPRINT("{ Block %d: size = %zu, offset = %zu, inUse = %s, id: %d}, ", i,
               block->size, block->offset, block->inUse ? "true" : "false",
               block->id);
    }
    DPRINT("\n");
}

static uint32_t
findMemoryType(const Onyx_Memory* memory, uint32_t memoryTypeBitsRequirement)
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
alignmentForBufferUsage(const Onyx_Memory* memory, VkBufferUsageFlags flags)
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
initBlockChain(Onyx_Memory* memory, const Onyx_MemoryType memType,
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
    chain->alignment = 4;
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
    if (memType == ONYX_MEMORY_EXTERNAL_DEVICE_TYPE)
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
#ifndef ONYX_NO_BUFFER_DEVICE_ADDRESS
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
#endif
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
        case ONYX_MEMORY_HOST_GRAPHICS_TYPE:
            queueFamilyIndex = memory->instance->graphicsQueueFamily.index;
            break;
        case ONYX_MEMORY_HOST_TRANSFER_TYPE:
            queueFamilyIndex = memory->instance->transferQueueFamily.index;
            break;
        case ONYX_MEMORY_DEVICE_TYPE:
            queueFamilyIndex = memory->instance->graphicsQueueFamily.index;
            break;
        case ONYX_MEMORY_EXTERNAL_DEVICE_TYPE:
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
            // TODO this sharing mode is why we need to expand Onyx_V_MemoryType
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

#ifndef ONYX_NO_BUFFER_DEVICE_ADDRESS
        const VkBufferDeviceAddressInfo addrInfo = {
            .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = chain->buffer};

        chain->bufferAddress =
            vkGetBufferDeviceAddress(memory->instance->device, &addrInfo);
#endif

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
freeBlockChain(Onyx_Memory* memory, struct BlockChain* chain)
{
    memset(chain->blocks, 0, MAX_BLOCKS * sizeof(Onyx_MemBlock));
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
        const Onyx_MemBlock* block = &chain->blocks[i];
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
        Onyx_MemBlock temp = chain->blocks[i];
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
        Onyx_MemBlock temp = chain->blocks[i];
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
        Onyx_MemBlock* curr = &chain->blocks[i];
        Onyx_MemBlock* next = &chain->blocks[i + 1];
        if (!curr->inUse && !next->inUse)
        {
            // combine them together
            const VkDeviceSize cSize = curr->size + next->size;
            // we could make the other id available for re-use as well
            curr->size               = cSize;
            // set next block to 0
            memset(next, 0, sizeof(Onyx_MemBlock));
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
        const Onyx_MemBlock* curr = &chain->blocks[i];
        const Onyx_MemBlock* next = &chain->blocks[i + 1];
        if (curr->offset > next->offset)
            return false;
    }
    return true;
}

static Onyx_MemBlock*
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
    Onyx_MemBlock* curBlock = &chain->blocks[availInfo.index];
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
    Onyx_MemBlock*   newBlock = &chain->blocks[newBlockIndex];
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
    Onyx_MemBlock*   block = &chain->blocks[blockIndex];
    const VkDeviceSize size  = block->size;
    block->inUse             = false;
    mergeBlocks(chain);
    chain->usedSize -= size;
    DPRINT(">> Freeing block %d of size %09zu from chain %s. %zu bytes out of "
           "%zu now in use.\n",
           id, size, chain->name, chain->usedSize, chain->totalSize);
}

void
onyx_CreateMemory(const Onyx_Instance* instance, const uint32_t hostGraphicsBufferMB,
                  const uint32_t deviceGraphicsBufferMB,
                  const uint32_t deviceGraphicsImageMB, const uint32_t hostTransferBufferMB,
                  const uint32_t deviceExternalGraphicsImageMB, Onyx_Memory* memory)
{
    assert(hostGraphicsBufferMB < 10000); // maximum 10 GB
    assert(deviceGraphicsBufferMB < 10000); // maximum 10 GB
    assert(deviceGraphicsImageMB < 10000); // maximum 10 GB
    assert(hostTransferBufferMB < 10000); // maximum 10 GB
    assert(deviceExternalGraphicsImageMB < 10000); // maximum 10 GB
    memset(memory, 0, sizeof(Onyx_Memory));
    memory->instance                 = instance;

    memory->deviceProperties = onyx_GetPhysicalDeviceProperties(instance);

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
#ifndef ONYX_NO_BUFFER_DEVICE_ADDRESS
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
#endif
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBufferUsageFlags hostTransferFlags =
#ifndef ONYX_NO_BUFFER_DEVICE_ADDRESS
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
#endif
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBufferUsageFlags devBufFlags =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
#ifndef ONYX_NO_BUFFER_DEVICE_ADDRESS
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
#endif
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    DPRINT("Onyx: Initializing memory block chains...\n");
    initBlockChain(memory, ONYX_MEMORY_HOST_GRAPHICS_TYPE,
                   hostGraphicsBufferMB * MB,
                   memory->hostVisibleCoherentTypeIndex, hostGraphicsFlags,
                   true, "hstGraphBuffer",
                   &memory->blockChainHostGraphicsBuffer);
    initBlockChain(memory, ONYX_MEMORY_HOST_TRANSFER_TYPE,
                   hostTransferBufferMB * MB,
                   memory->hostVisibleCoherentTypeIndex, hostTransferFlags,
                   true, "hstTransBuffer",
                   &memory->blockChainHostTransferBuffer);
    initBlockChain(memory, ONYX_MEMORY_DEVICE_TYPE,
                   deviceGraphicsBufferMB * MB,
                   memory->deviceLocalTypeIndex, devBufFlags, false,
                   "devBuffer", &memory->blockChainDeviceGraphicsBuffer);
    initBlockChain(memory, ONYX_MEMORY_DEVICE_TYPE,
                   deviceGraphicsImageMB * MB,
                   memory->deviceLocalTypeIndex, 0, false, "devImage",
                   &memory->blockChainDeviceGraphicsImage);
    initBlockChain(memory, ONYX_MEMORY_EXTERNAL_DEVICE_TYPE,
                   deviceExternalGraphicsImageMB * MB,
                   memory->deviceLocalTypeIndex, 0, false, "devExtImage",
                   &memory->blockChainExternalDeviceGraphicsImage);
}

Onyx_BufferRegion
onyx_RequestBufferRegionAligned(Onyx_Memory* memory, const size_t size,
                                uint32_t                alignment,
                                const Onyx_MemoryType memType)
{
    assert(size > 0);
    if (size % 4 != 0) // only allow for word-sized blocks
    {
        hell_Error(HELL_ERR_FATAL, "Size %zu is not 4 byte aligned.", size);
    }
    Onyx_MemBlock*   block = NULL;
    struct BlockChain* chain = NULL;
    switch (memType)
    {
    case ONYX_MEMORY_HOST_GRAPHICS_TYPE:
        chain = &memory->blockChainHostGraphicsBuffer;
        break;
    case ONYX_MEMORY_HOST_TRANSFER_TYPE:
        chain = &memory->blockChainHostTransferBuffer;
        break;
    case ONYX_MEMORY_DEVICE_TYPE:
        chain = &memory->blockChainDeviceGraphicsBuffer;
        break;
    default:
        block = NULL;
        assert(0);
    }

    assert(hell_is_power_of_two(alignment));

    if (0 == alignment)
        alignment = chain->alignment;
    // i want to move to a model where we have a separate chain for each
    // alignment type. the main reason is for resizing: if i need to resize a
    // region I need to know what alignment it requires. I don't want the user
    // to have to provide that again. But I also don't want to store an
    // alignment along with every region. So for now, I just make sure the chain
    // alignment is as large as the largest alignment requirement it has
    // allocated for. since each is a power of 2, then it implicitly will
    // satisfy all of its regions alignment reqs.
    else if (alignment > chain->alignment)
        chain->alignment = alignment;
    block = requestBlock(size, alignment, chain);

    Onyx_BufferRegion region = {0};
    region.offset              = block->offset;
    region.memBlockId          = block->id;
    region.size                = size;
    region.buffer              = chain->buffer;
    region.pChain              = chain;

    // TODO this check is very brittle. we should probably change V_MemoryType
    // to a mask with flags for the different requirements. One bit for host or
    // device, one bit for transfer capabale, one bit for external, one bit for
    // image or buffer
    if (memType == ONYX_MEMORY_HOST_TRANSFER_TYPE ||
        memType == ONYX_MEMORY_HOST_GRAPHICS_TYPE)
        region.hostData = chain->hostData + block->offset;
    else
        region.hostData = NULL;

    return region;
}

Onyx_BufferRegion
onyx_RequestBufferRegion(Onyx_Memory* memory, const size_t size,
                         const VkBufferUsageFlags flags,
                         const Onyx_MemoryType  memType)
{
    uint32_t alignment = alignmentForBufferUsage(memory, flags);
    return onyx_RequestBufferRegionAligned(memory,
        size, alignment,
        memType); // TODO: fix this. find the maximum alignment and choose that
}

Onyx_BufferRegion
onyx_RequestBufferRegionArray(Onyx_Memory* memory, uint32_t elemSize, uint32_t elemCount,
                              VkBufferUsageFlags flags,
                              Onyx_MemoryType  memType)
{
    // calculate the size needed and call requestBufferRegionAligned.
    // make sure to set the stride.
    uint32_t alignment = alignmentForBufferUsage(memory, flags);
    uint32_t stride    = hell_Align(elemSize, alignment);
    uint64_t size      = stride * elemCount;
    Onyx_BufferRegion region = onyx_RequestBufferRegionAligned(memory, size, alignment, memType);
    region.stride = stride;
    return region;
}

uint32_t
onyx_GetMemoryType(const Onyx_Memory* memory, uint32_t typeBits,
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

Onyx_Image
onyx_CreateImage(Onyx_Memory* memory, const uint32_t width,
                 const uint32_t height, const VkFormat format,
                 const VkImageUsageFlags  usageFlags,
                 const VkImageAspectFlags aspectMask,
                 const VkSampleCountFlags sampleCount, const uint32_t mipLevels,
                 const Onyx_MemoryType memType)
{
    assert(mipLevels > 0);
    assert(memType == ONYX_MEMORY_DEVICE_TYPE ||
           memType == ONYX_MEMORY_EXTERNAL_DEVICE_TYPE);

    assert(memory->deviceProperties->limits.framebufferColorSampleCounts >=
           sampleCount);
    assert(memory->deviceProperties->limits.framebufferDepthSampleCounts >=
           sampleCount);

    void* pNext = NULL;

    VkExternalMemoryImageCreateInfo externalImageInfo;
    if (memType == ONYX_MEMORY_EXTERNAL_DEVICE_TYPE)
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

    Onyx_Image image = {0};

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
    case ONYX_MEMORY_DEVICE_TYPE:
        image.pChain = &memory->blockChainDeviceGraphicsImage;
        break;
    case ONYX_MEMORY_EXTERNAL_DEVICE_TYPE:
        image.pChain = &memory->blockChainExternalDeviceGraphicsImage;
        break;
    default:
        assert(0);
    }

    const Onyx_MemBlock* block =
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
onyx_FreeImage(Onyx_Image* image)
{
    assert(image->size != 0);
    if (image->sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(image->pChain->memory->instance->device, image->sampler, NULL);
    }
    vkDestroyImageView(image->pChain->memory->instance->device, image->view, NULL);
    vkDestroyImage(image->pChain->memory->instance->device, image->handle, NULL);
    freeBlock(image->pChain, image->memBlockId);
    memset(image, 0, sizeof(Onyx_Image));
}

void
onyx_FreeBufferRegion(Onyx_BufferRegion* pRegion)
{
    assert(pRegion->size != 0);
    freeBlock(pRegion->pChain, pRegion->memBlockId);
    if (pRegion->hostData)
        memset(pRegion->hostData, 0, pRegion->size);
    memset(pRegion, 0, sizeof(Onyx_BufferRegion));
}

void
onyx_CopyBufferRegion(const Onyx_BufferRegion* src,
                        Onyx_BufferRegion*       dst)
{
    Onyx_Command cmd =
        onyx_CreateCommand(src->pChain->memory->instance, ONYX_V_QUEUE_GRAPHICS_TYPE); // arbitrary index;

    onyx_BeginCommandBuffer(cmd.buffer);

    VkBufferCopy copy;
    copy.srcOffset = src->offset;
    copy.dstOffset = dst->offset;
    copy.size      = src->size;

    vkCmdCopyBuffer(cmd.buffer, src->buffer, dst->buffer, 1, &copy);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_DestroyCommand(cmd);
}

void
onyx_v_CopyImageToBufferRegion(const Onyx_Image*  image,
                               Onyx_BufferRegion* bufferRegion)
{
    // TODO
}

void
onyx_TransferToDevice(Onyx_Memory* memory, Onyx_BufferRegion* pRegion)
{
    const Onyx_BufferRegion srcRegion = *pRegion;
    assert(srcRegion.pChain ==
           &memory->blockChainHostGraphicsBuffer); // only chain it makes sense
                                                   // to transfer from
    Onyx_BufferRegion destRegion = onyx_RequestBufferRegion(memory,
        srcRegion.size, 0, ONYX_MEMORY_DEVICE_TYPE);

    onyx_CopyBufferRegion(&srcRegion, &destRegion);

    onyx_FreeBufferRegion(pRegion);

    *pRegion = destRegion;
}

void
onyx_DestroyMemory(Onyx_Memory* memory)
{
    freeBlockChain(memory, &memory->blockChainHostGraphicsBuffer);
    freeBlockChain(memory, &memory->blockChainHostTransferBuffer);
    freeBlockChain(memory, &memory->blockChainDeviceGraphicsImage);
    freeBlockChain(memory, &memory->blockChainDeviceGraphicsBuffer);
    freeBlockChain(memory, &memory->blockChainExternalDeviceGraphicsImage);
}

VkDeviceAddress
onyx_GetBufferRegionAddress(const BufferRegion* region)
{
    assert(region->pChain->bufferAddress != 0);
    return region->pChain->bufferAddress + region->offset;
}

void
onyx_CreateUnmanagedBuffer(Onyx_Memory*             memory,
                           const VkBufferUsageFlags bufferUsageFlags,
                           const uint32_t           memorySize,
                           const Onyx_MemoryType  type,
                           VkDeviceMemory* pMemory, VkBuffer* pBuffer)
{
    uint32_t typeIndex;
    switch (type)
    {
    case ONYX_MEMORY_HOST_GRAPHICS_TYPE:
        typeIndex = memory->hostVisibleCoherentTypeIndex;
        break;
    case ONYX_MEMORY_HOST_TRANSFER_TYPE:
        typeIndex = memory->hostVisibleCoherentTypeIndex;
        break;
    case ONYX_MEMORY_DEVICE_TYPE:
        typeIndex = memory->deviceLocalTypeIndex;
        break;
    case ONYX_MEMORY_EXTERNAL_DEVICE_TYPE:
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

VkDeviceMemory
onyx_GetDeviceMemory(const Onyx_Memory* memory, const Onyx_MemoryType memType)
{
    switch (memType)
    {
    case ONYX_MEMORY_EXTERNAL_DEVICE_TYPE:
        return memory->blockChainExternalDeviceGraphicsImage.vkmemory;
    default:
        assert(0); // TODO
        return 0;
    }
}

VkDeviceSize
onyx_GetMemorySize(const Onyx_Memory* memory, const Onyx_MemoryType memType)
{
    switch (memType)
    {
    case ONYX_MEMORY_EXTERNAL_DEVICE_TYPE:
        return memory->blockChainExternalDeviceGraphicsImage.totalSize;
    default:
        assert(0); // TODO
        return 0;
    }
}

#ifdef WIN32
bool onyx_GetExternalMemoryWin32Handle(const Onyx_Memory* memory, HANDLE* handle, uint64_t* size)
{
    VkMemoryGetWin32HandleInfoKHR handleInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
        .memory = onyx_GetDeviceMemory(memory, ONYX_MEMORY_EXTERNAL_DEVICE_TYPE),
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
    };

    V_ASSERT( vkGetMemoryWin32HandleKHR(onyx_GetDevice(memory->instance), &handleInfo, handle) );

    *size = onyx_GetMemorySize(memory, ONYX_MEMORY_EXTERNAL_DEVICE_TYPE);

    assert(*size);
    return true;
}
#else
bool onyx_GetExternalMemoryFd(const Onyx_Memory* memory, int* fd, uint64_t* size)
{
    // fast path
    VkMemoryGetFdInfoKHR fdInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
        .memory = onyx_GetDeviceMemory(memory, ONYX_MEMORY_EXTERNAL_DEVICE_TYPE),
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
    };

    V_ASSERT( vkGetMemoryFdKHR(onyx_GetDevice(memory->instance), &fdInfo, fd) );

    *size = onyx_GetMemorySize(memory, ONYX_MEMORY_EXTERNAL_DEVICE_TYPE);

    assert(*size);
    return true;
}
#endif

uint64_t
onyx_SizeOfMemory(void)
{
    return sizeof(Onyx_Memory);
}

Onyx_Memory* onyx_AllocMemory(void)
{
    return hell_Malloc(sizeof(Onyx_Memory));
}

static void
simpleBlockchainReport(const BlockChain* chain)
{
    float percent = (float)chain->usedSize / chain->totalSize;
    hell_Print("Blockchain: %s Used Size: %d Total Size: %d Percent Used: %f\n", chain->name, chain->usedSize, chain->totalSize, percent);
}

void
onyx_MemoryReportSimple(const Onyx_Memory* memory)
{
    hell_Print("Memory Report\n");
    simpleBlockchainReport(&memory->blockChainHostGraphicsBuffer);
    simpleBlockchainReport(&memory->blockChainDeviceGraphicsBuffer);
    simpleBlockchainReport(&memory->blockChainDeviceGraphicsImage);
    simpleBlockchainReport(&memory->blockChainHostTransferBuffer);
    simpleBlockchainReport(&memory->blockChainExternalDeviceGraphicsImage);
}

void
onyx_GetImageMemoryUsage(const Onyx_Memory* memory, uint64_t* bytes_in_use, uint64_t* total_bytes)
{
    *bytes_in_use = memory->blockChainDeviceGraphicsImage.usedSize;
    *total_bytes = memory->blockChainDeviceGraphicsImage.totalSize;
}

const Onyx_Instance* onyx_GetMemoryInstance(const Onyx_Memory* memory)
{
    return memory->instance;
}

#define error hell_error_fatal

static void
growBufferRegion(BufferRegion* region, size_t new_size)
{
    BlockChain* chain = region->pChain;
    uint32_t cur_id = region->memBlockId;
    Block* block = NULL;

    for (int i = 0; i < chain->count; i++) {
        if (chain->blocks[i].id == cur_id) {
            block = &chain->blocks[i];
            break;
        }
    }
    assert(block);

    if (new_size <= block->size) {
        // can trivially set the region size to new_size and return
        region->size = new_size;
        return;
    }

    Block*       new_block  = requestBlock(new_size, chain->alignment, chain);
    BufferRegion new_region = *region;
    new_region.offset       = new_block->offset;
    new_region.memBlockId   = new_block->id;
    new_region.size         = new_size;

    // is host mapped
    if (new_region.hostData) {
        new_region.hostData = chain->hostData + new_block->offset;
        memcpy(new_region.hostData, region->hostData, region->size);
    } else {
        error("GPU resident grow region not implemented yet :(");
    }

    onyx_FreeBufferRegion(region);
    *region = new_region;
}

void
onyx_ResizeBufferRegion(BufferRegion* region, size_t new_size)
{
    if (new_size > region->size)
        growBufferRegion(region, new_size);
    else
        hell_error_fatal("Shrinking buffer is not supported yet :(");
}
