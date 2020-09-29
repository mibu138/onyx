#include "v_memory.h"
#include "v_video.h"
#include "def.h"
#include <stdio.h>
#include <vulkan/vulkan_core.h>
#include <assert.h>

// HVC = Host Visible and Coherent
#define MEMORY_SIZE_HVC 16777216
#define BUFFER_SIZE_HVC 16777216 // 2 MiB
// DL = Device Local    
#define MEMORY_SIZE_DL  33554432 // 32 MiB
#define MAX_BLOCKS 256

static VkDeviceMemory memoryHostVisibleCoherent;
static VkDeviceMemory memoryDeviceLocal;
static VkBuffer       bufferHostMapped;
uint8_t*              hostBuffer;

static VkPhysicalDeviceMemoryProperties memoryProperties;
static V_block blocks[MAX_BLOCKS];

static void printBufferMemoryReqs(const VkMemoryRequirements* reqs)
{
    printf("Size: %ld\tAlignment: %ld\n", reqs->size, reqs->alignment);
}

void v_InitMemory(void)
{
    VkResult r;

    int hostVisibleCoherentTypeIndex;
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

    const VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MEMORY_SIZE_HVC,
        .memoryTypeIndex = hostVisibleCoherentTypeIndex
    };

    r = vkAllocateMemory(device, &allocInfo, NULL, &memoryHostVisibleCoherent);
    assert( VK_SUCCESS == r );

    VkBufferCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // queue determined by first use
        .size = BUFFER_SIZE_HVC
    };


    r = vkCreateBuffer(device, &ci, NULL, &bufferHostMapped);
    assert( VK_SUCCESS == r );

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(device, bufferHostMapped, &reqs);
    // we dont need to check the reqs. spec states that 
    // any buffer created without SPARSITY flags will 
    // support being bound to host visible | host coherent

    r = vkBindBufferMemory(device, bufferHostMapped, memoryHostVisibleCoherent, 0);
    assert( VK_SUCCESS == r );

    r = vkMapMemory(device, memoryHostVisibleCoherent, 0, BUFFER_SIZE_HVC, 0, (void**)&hostBuffer);
    assert( VK_SUCCESS == r );

    // --------------------------------------------------------
    // allocate device local memory
    // --------------------------------------------------------

    const VkMemoryAllocateInfo allocInfoDl = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MEMORY_SIZE_DL,
        .memoryTypeIndex = deviceLocalTypeIndex
    };

    r = vkAllocateMemory(device, &allocInfoDl, NULL, &memoryDeviceLocal);
    assert( VK_SUCCESS == r );
}

V_block* v_RequestBlockAligned(const size_t size, const uint32_t alignment)
{
    static int blockCount = 0;
    static int bytesAvailable = BUFFER_SIZE_HVC;
    static int curBufferOffset = 0;
    assert( size % 4 == 0 ); // only allow for word-sized blocks
    assert( size < bytesAvailable);
    assert( blockCount < MAX_BLOCKS );
    if (curBufferOffset % alignment != 0)
        curBufferOffset = (curBufferOffset / alignment + 1) * alignment;
    V_block* pBlock = &blocks[blockCount];
    pBlock->address = hostBuffer + curBufferOffset;
    pBlock->vBuffer = &bufferHostMapped;
    pBlock->size = size;
    pBlock->vOffset = curBufferOffset;
    pBlock->isMapped = true;

    bytesAvailable -= size;
    curBufferOffset += size;
    blockCount++;
    // we really do need to be worrying about alignment here.
    // anything that is not a multiple of 4 bytes will have issues.
    // there is VERY GOOD CHANCE that there are other alignment
    // issues to consider.
    //
    // we should probably divide up the buffer into Chunks, where
    // all the blocks in a chunk contain the same kind of element
    // (a chunk for vertices, a chunk for indices, a chunk for 
    // uniform matrices, etc).

    return pBlock;
}

V_block* v_RequestBlock(const size_t size, const VkBufferUsageFlags flags)
{
    uint32_t alignment;
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
    return v_RequestBlockAligned(size, alignment);
}

V_block* v_RequestBlockDevLocal(const VkMemoryRequirements2 memReqs)
{
}

uint32_t v_GetMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags properties)
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


void v_BindImageToMemory(const VkImage image, const uint32_t size)
{
    //static bool imageBound = false;
    //assert (!imageBound);
    static uint32_t curDevMemoryOffset = 0;
    assert( curDevMemoryOffset < MEMORY_SIZE_DL );
    vkBindImageMemory(device, image, memoryDeviceLocal, curDevMemoryOffset);
    curDevMemoryOffset += size;
}

void v_CleanUpMemory()
{
    vkUnmapMemory(device, memoryHostVisibleCoherent);
    vkDestroyBuffer(device, bufferHostMapped, NULL);
    vkFreeMemory(device, memoryHostVisibleCoherent, NULL);
    vkFreeMemory(device, memoryDeviceLocal, NULL);
};
