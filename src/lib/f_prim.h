#ifndef OBSIDIAN_F_PRIM_H
#define OBSIDIAN_F_PRIM_H

#include <stdint.h>

typedef struct {
    uint32_t  attrCount;
    uint32_t  vertexCount;
    uint32_t  indexCount;
    uint32_t  padding;
    uint8_t*  attrSizes;
    char**    attrNames;
    void**    attributes;
    uint32_t* indices;
} Obdn_F_Primitive;


#endif /* end of include guard: OBSIDIAN_F_PRIM_H */

