#ifndef ONYX_V_PRIVATE_H
#define ONYX_V_PRIVATE_H

#include "vulkan.h"

#define MAX_BLOCKS 1000

typedef struct Onyx_MemBlock {
    VkDeviceSize   size; // note this is the actual available space at this block. the resource might not be using all of it.
    VkDeviceSize   offset;
    _Bool          inUse;
    uint32_t       id; // global unique identifier
} Onyx_MemBlock;

typedef struct BlockChain {
    char                 name[16]; // for debugging
    VkDeviceSize         totalSize;
    VkDeviceSize         usedSize;
    VkDeviceSize         defaultAlignment;
    VkDeviceAddress      bufferAddress;
    uint32_t             count;
    uint32_t             cur;
    uint32_t             nextBlockId;
    VkDeviceMemory       vkmemory;
    VkBuffer             buffer;
    VkBufferUsageFlags   bufferFlags;
    uint8_t*             hostData;
    Onyx_MemBlock        blocks[MAX_BLOCKS];
    struct Onyx_Memory*  memory;
} BlockChain;

typedef struct Onyx_Instance Onyx_Instance;

typedef struct Onyx_Memory {
    VkPhysicalDeviceMemoryProperties properties;
    BlockChain                       blockChainHostGraphicsBuffer;
    BlockChain                       blockChainDeviceGraphicsBuffer;
    BlockChain                       blockChainDeviceGraphicsImage;
    BlockChain                       blockChainHostTransferBuffer;
    BlockChain                       blockChainExternalDeviceGraphicsImage;

    const VkPhysicalDeviceProperties* deviceProperties;

    uint32_t hostVisibleCoherentTypeIndex;
    uint32_t deviceLocalTypeIndex;

    const Onyx_Instance* instance;
} Onyx_Memory;


#endif /* end of include guard: ONYX_V_PRIVATE_H */
