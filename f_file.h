#ifndef TANTO_F_FILE_H
#define TANTO_F_FILE_H

#include <stdint.h>
#include "r_geo.h"

typedef struct {
    uint32_t  attrCount;
    uint32_t  vertexCount;
    uint32_t  indexCount;
    uint32_t  padding;
    uint8_t*  attrSizes;
    char**    attrNames;
    void**    attributes;
    Tanto_R_Index*      indices;
} Tanto_F_Primitive;

// Must be freed
Tanto_F_Primitive tanto_f_CreatePrimitive(const uint32_t vertexCount, const uint32_t indexCount, 
        const uint32_t attrCount, const Tanto_R_AttributeSize attrSizes[attrCount]);
Tanto_F_Primitive tanto_f_CreateFPrimFromRPrim(const Tanto_R_Primitive* rprim);
Tanto_R_Primitive tanto_f_CreateRPrimFromFPrim(const Tanto_F_Primitive* fprim);
int               tanto_f_WritePrimitive(const char* filename, const Tanto_F_Primitive* fprim);
int               tanto_f_ReadPrimitive(const char* filename, Tanto_F_Primitive* fprim);
void              tanto_f_FreePrimitive(Tanto_F_Primitive* fprim);

#endif /* end of include guard: TANTO_F_FILE_H */
