#ifndef OBSIDIAN_V_PRIVATE_H
#define OBSIDIAN_V_PRIVATE_H

#include "vulkan.h"

#define MAX_QUEUES 32
#define MAX_BLOCKS 1000

typedef struct Obdn_QueueFamily {
    uint32_t index;
    uint32_t queueCount;
    VkQueue  queues[MAX_QUEUES];
} Obdn_QueueFamily;

typedef Obdn_QueueFamily QueueFamily;

typedef struct Obdn_Instance {
    VkInstance                                         vkinstance;
    VkPhysicalDevice                                   physicalDevice;
    VkDevice                                           device;
    Obdn_QueueFamily                                   graphicsQueueFamily;
    Obdn_QueueFamily                                   computeQueueFamily;
    Obdn_QueueFamily                                   transferQueueFamily;
    VkQueue                                            presentQueue;
    VkDebugUtilsMessengerEXT                           debugMessenger;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR    rtProperties;
    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelStructProperties;
    VkPhysicalDeviceProperties                         deviceProperties;
} Obdn_Instance;

typedef struct obdn_V_MemBlock {
    VkDeviceSize   size;
    VkDeviceSize   offset;
    _Bool          inUse;
    uint32_t       id; // global unique identifier
} Obdn_V_MemBlock;

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
    Obdn_V_MemBlock      blocks[MAX_BLOCKS];
    struct Obdn_Memory*  memory;
} BlockChain;

typedef struct Obdn_Memory {
    VkPhysicalDeviceMemoryProperties properties;
    BlockChain                       blockChainHostGraphicsBuffer;
    BlockChain                       blockChainDeviceGraphicsBuffer;
    BlockChain                       blockChainDeviceGraphicsImage;
    BlockChain                       blockChainHostTransferBuffer;
    BlockChain                       blockChainExternalDeviceGraphicsImage;

    const VkPhysicalDeviceProperties* deviceProperties;

    uint32_t hostVisibleCoherentTypeIndex;
    uint32_t deviceLocalTypeIndex;

    const Obdn_Instance* instance;
} Obdn_Memory;


#endif /* end of include guard: OBSIDIAN_V_PRIVATE_H */
