#ifndef TANTO_F_FILE_H
#define TANTO_F_FILE_H

#include <stdint.h>
#include "r_geo.h"

typedef struct {
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t attrCount;
    Tanto_R_Attribute** attributes;
    Tanto_R_Index*      indices;
} Tanto_F_Primitive;

Tanto_F_Primitive tanto_f_CreateFPrimFromRPrim(const Tanto_R_Primitive* rprim);

#endif /* end of include guard: TANTO_F_FILE_H */
