#ifndef V_MEMORY_H
#define V_MEMORY_H

#include "v_def.h"
#include <stdbool.h>

typedef struct {
    size_t       size;
    uint8_t*     address;
    VkDeviceSize vOffset;
    VkBuffer*    vBuffer;
    bool         isMapped;
} V_block;

extern uint8_t* hostBuffer;

//typedef enum {
//    V_MEM_TYPE_VERTEX, 
//    V_MEM_TYPE_INDEX,
//    V_MEM_TYPE_UNIFORM, 
//} V_MEM_TYPE;

void v_InitMemory(void);

V_block* v_RequestBlock(size_t size, const VkBufferUsageFlags);

V_block* v_RequestBlockAligned(const size_t size, const uint32_t alignment);

uint32_t v_GetMemoryType(uint32_t typeBits, const VkMemoryPropertyFlags properties);

void v_BindImageToMemory(const VkImage, const uint32_t size);

void v_CleanUpMemory(void);

#endif /* end of include guard: V_MEMORY_H */
