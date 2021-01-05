#include "f_file.h"
#include <stdlib.h>

Tanto_F_Primitive tanto_f_CreateFPrimFromRPrim(const Tanto_R_Primitive* rprim)
{
    Tanto_F_Primitive fprim = {
        .vertexCount = rprim->vertexCount,
        .indexCount  = rprim->indexCount,
        .attrCount   = rprim->attrCount
    };

    const uint32_t attrCount = rprim->attrCount;
    const uint32_t vertCount = rprim->vertexCount;

    assert(attrCount < TANTO_R_MAX_VERT_ATTRIBUTES);

    Tanto_R_Attribute* attributes[attrCount];

    for (int i = 0; i < attrCount; i++) 
    {
        attributes[i] = malloc(vertCount * sizeof(Tanto_R_Attribute));
    }

    if (!rprim->vertexRegion.hostData)
    {
        // must copy data to host
    }
}
