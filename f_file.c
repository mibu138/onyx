#include "f_file.h"
#include "tanto/r_geo.h"
#include "tanto/v_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Tanto_F_Primitive tanto_f_CreatePrimitive(const uint32_t vertexCount, const uint32_t indexCount, const uint32_t attrCount)
{
    Tanto_F_Primitive fprim = {
        .vertexCount = vertexCount,
        .indexCount  = indexCount,
        .attrCount   = attrCount
    };

    const size_t attributeChunkSize = vertexCount * sizeof(Tanto_R_Attribute);
    const size_t indexChunkSize     = indexCount * sizeof(Tanto_R_Index);

    fprim.indices =    malloc(indexChunkSize); 
    fprim.attributes = malloc(attrCount * sizeof(Tanto_R_Attribute*));

    for (int i = 0; i < attrCount; i++) 
    {
        fprim.attributes[i] = malloc(attributeChunkSize);
    }

    return fprim;
}

Tanto_F_Primitive tanto_f_CreateFPrimFromRPrim(const Tanto_R_Primitive* rprim)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    Tanto_F_Primitive fprim = {
        .vertexCount = rprim->vertexCount,
        .indexCount  = rprim->indexCount,
        .attrCount   = rprim->attrCount
    };

    const uint32_t attrCount = rprim->attrCount;
    const uint32_t vertCount = rprim->vertexCount;
    const uint32_t indexCount = rprim->indexCount;
    const size_t attributeChunkSize = vertCount * sizeof(Tanto_R_Attribute);
    const size_t indexChunkSize     = indexCount * sizeof(Tanto_R_Index);

    assert(attrCount < TANTO_R_MAX_VERT_ATTRIBUTES);

    fprim.indices =    malloc(indexChunkSize); 
    fprim.attributes = malloc(attrCount * sizeof(Tanto_R_Attribute*));

    for (int i = 0; i < attrCount; i++) 
    {
        fprim.attributes[i] = malloc(attributeChunkSize);
    }

    Tanto_V_BufferRegion hostVertRegion;
    Tanto_V_BufferRegion hostIndexRegion;

    if (!rprim->vertexRegion.hostData)
    {
        // must copy data to host
        hostVertRegion = tanto_v_RequestBufferRegion(attrCount * attributeChunkSize,
                0, TANTO_V_MEMORY_HOST_TYPE);
        hostIndexRegion = tanto_v_RequestBufferRegion(indexChunkSize,
                0, TANTO_V_MEMORY_HOST_TYPE);
        tanto_v_CopyBufferRegion(&rprim->vertexRegion, &hostVertRegion);
        tanto_v_CopyBufferRegion(&rprim->indexRegion, &hostIndexRegion);
    }
    else 
    {
        hostVertRegion = rprim->vertexRegion;
        hostIndexRegion = rprim->indexRegion;
    }

    for (int i = 0; i < attrCount; i++) 
    {
        const size_t offset = i * attributeChunkSize;
        const uint8_t* src = hostVertRegion.hostData + offset;
        memcpy(fprim.attributes[i], src, attributeChunkSize);
    }
    memcpy(fprim.indices, hostIndexRegion.hostData, indexChunkSize);

    if (!rprim->vertexRegion.hostData)
    {
        tanto_v_FreeBufferRegion(&hostVertRegion);
        tanto_v_FreeBufferRegion(&hostIndexRegion);
    }

    return fprim;
}

Tanto_R_Primitive tanto_f_CreateRPrimFromFPrim(const Tanto_F_Primitive *fprim)
{
    Tanto_R_Primitive rprim = tanto_r_CreatePrimitive(fprim->vertexCount, fprim->indexCount, fprim->attrCount);
    const size_t vertexDataSize = fprim->vertexCount * sizeof(Tanto_R_Attribute);
    const size_t indexDataSize  = fprim->indexCount * sizeof(Tanto_R_Index);
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        Tanto_R_Attribute* dst = tanto_r_GetPrimAttribute(&rprim, i);
        memcpy(dst, fprim->attributes[i], vertexDataSize);
    }
    memcpy(rprim.indexRegion.hostData, fprim->indices, indexDataSize);
    return rprim;
}

int tanto_f_WritePrimitive(const char* filename, const Tanto_F_Primitive* fprim)
{
    FILE* file = fopen(filename, "wb");
    assert(file);
    const size_t headerSize   = sizeof(Tanto_F_Primitive) - sizeof(Tanto_R_Attribute**) - sizeof(Tanto_R_Index*);
    const size_t attrDataSize = sizeof(Tanto_R_Attribute) * fprim->vertexCount;
    const size_t indexDataSize = sizeof(Tanto_R_Index) * fprim->indexCount;
    size_t r;
    r = fwrite(fprim, headerSize, 1, file);
    assert(r == 1);
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        r = fwrite(fprim->attributes[i], attrDataSize, 1, file);
        assert(r == 1);
    }
    r = fwrite(fprim->indices, indexDataSize, 1, file);
    assert(r == 1);
    r = fclose(file);
    assert(r == 0);
    return 1;
}

int tanto_f_ReadPrimitive(const char* filename, Tanto_F_Primitive* fprim)
{
    FILE* file = fopen(filename, "rb");
    assert(file);
    size_t r;
    const size_t headerSize   = sizeof(Tanto_F_Primitive) - sizeof(Tanto_R_Attribute**) - sizeof(Tanto_R_Index*);
    r = fread(fprim, headerSize, 1, file);
    assert(r == 1);
    const size_t attrDataSize = sizeof(Tanto_R_Attribute) * fprim->vertexCount;
    const size_t indexDataSize = sizeof(Tanto_R_Index) * fprim->indexCount;
    fprim->attributes = malloc(fprim->attrCount * sizeof(Tanto_R_Attribute*));
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        fprim->attributes[i] = malloc(attrDataSize);
        r = fread(fprim->attributes[i], attrDataSize, 1, file);
        assert(r);
    }
    fprim->indices = malloc(indexDataSize);
    fread(fprim->indices, indexDataSize, 1, file);
    assert(r == 1);
    return 1;
}

void tanto_f_FreePrimitive(Tanto_F_Primitive* fprim)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        free(fprim->attributes[i]);
    }
    free(fprim->attributes);
    free(fprim->indices);
    memset(fprim, 0, sizeof(Tanto_F_Primitive));
}
