#include "f_file.h"
#include "r_geo.h"
#include "tanto/v_memory.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(uint8_t) == sizeof(Tanto_R_AttributeSize), "sizeof(Tanto_R_AttributeSize) must be 1");

typedef Tanto_F_Primitive FPrim;

static void printPrim(const FPrim* prim)
{
    printf(">>> printing Fprim info...\n");
    printf(">>> attrCount %d vertexCount %d indexCount %d \n", prim->attrCount, prim->vertexCount, prim->indexCount);
    printf(">>> attrSizes ");
    for (int i = 0; i < prim->attrCount; i++) 
        printf("%d ", prim->attrSizes[i]);
    printf("\n>>> attrNames ");
    for (int i = 0; i < prim->attrCount; i++) 
        printf("%s ", prim->attrNames[i]);
    printf("\n>>> Attributes: \n");
    for (int i = 0; i < prim->attrCount; i++) 
    {
        printf(">>> %s: ", prim->attrNames[i]);
        for (int j = 0; j < prim->vertexCount; j++) 
        {
            printf("{");
            const int dim = prim->attrSizes[i]/4;
            const float* vals = &((float**)prim->attributes)[i][j * dim];
            for (int k = 0; k < dim; k++)
                printf("%f%s", vals[k], k == dim - 1 ? "" : ", ");
            printf("%s", j == prim->vertexCount - 1 ? "}" : "}, ");
        }
        printf("\n");
    }
    printf("Indices: ");
    for (int i = 0; i < prim->indexCount; i++) 
        printf("%d%s", prim->indices[i], i == prim->indexCount - 1 ? "" : ", ");
    printf("\n");
}

void tanto_f_PrintPrim(const Tanto_F_Primitive* prim)
{
    printPrim(prim);
}

Tanto_F_Primitive tanto_f_CreatePrimitive(const uint32_t vertexCount, const uint32_t indexCount, 
        const uint32_t attrCount, const Tanto_R_AttributeSize attrSizes[attrCount])
{
    Tanto_F_Primitive fprim = {
        .vertexCount = vertexCount,
        .indexCount  = indexCount,
        .attrCount   = attrCount
    };

    fprim.attrSizes  = malloc(attrCount * sizeof(Tanto_R_AttributeSize));
    fprim.indices    = malloc(indexCount * sizeof(Tanto_R_Index)); 
    fprim.attrNames  = malloc(attrCount * sizeof(char*));
    fprim.attributes = malloc(attrCount * sizeof(void*));

    for (int i = 0; i < attrCount; i++) 
    {
        fprim.attrSizes[i] = attrSizes[i];
        fprim.attrNames[i] = malloc(TANTO_R_ATTR_NAME_LEN);
        fprim.attributes[i] = malloc(vertexCount * attrSizes[i]);
    }

    return fprim;
}

Tanto_F_Primitive tanto_f_CreateFPrimFromRPrim(const Tanto_R_Primitive* rprim)
{
    Tanto_F_Primitive fprim = tanto_f_CreatePrimitive(rprim->vertexCount, rprim->indexCount, rprim->attrCount, rprim->attrSizes);

    Tanto_V_BufferRegion hostVertRegion;
    Tanto_V_BufferRegion hostIndexRegion;

    size_t attrDataSize = 0;
    for (int i = 0; i < rprim->attrCount; i++) {
        attrDataSize += rprim->vertexCount * rprim->attrSizes[i];
    }

    assert(attrDataSize > 0);

    if (!rprim->vertexRegion.hostData)
    {
        // must copy data to host
        hostVertRegion = tanto_v_RequestBufferRegion(attrDataSize,
                0, TANTO_V_MEMORY_HOST_GRAPHICS_TYPE);
        hostIndexRegion = tanto_v_RequestBufferRegion(rprim->indexCount * sizeof(Tanto_R_Index),
                0, TANTO_V_MEMORY_HOST_GRAPHICS_TYPE);
        tanto_v_CopyBufferRegion(&rprim->vertexRegion, &hostVertRegion);
        tanto_v_CopyBufferRegion(&rprim->indexRegion, &hostIndexRegion);
    }
    else 
    {
        hostVertRegion = rprim->vertexRegion;
        hostIndexRegion = rprim->indexRegion;
    }

    for (int i = 0; i < rprim->attrCount; i++) 
    {
        const size_t chunkSize = rprim->attrSizes[i] * rprim->vertexCount;
        const void* src = hostVertRegion.hostData + rprim->attrOffsets[i];
        memcpy(fprim.attributes[i], src, chunkSize);
        src = rprim->attrNames[i];
        memcpy(fprim.attrNames[i], src, TANTO_R_ATTR_NAME_LEN);
        fprim.attrSizes[i] = rprim->attrSizes[i];
    }

    memcpy(fprim.indices, hostIndexRegion.hostData, rprim->indexCount * sizeof(Tanto_R_Index));

    if (!rprim->vertexRegion.hostData)
    {
        tanto_v_FreeBufferRegion(&hostVertRegion);
        tanto_v_FreeBufferRegion(&hostIndexRegion);
    }

    return fprim;
}

Tanto_R_Primitive tanto_f_CreateRPrimFromFPrim(const Tanto_F_Primitive *fprim)
{
    Tanto_R_Primitive rprim = tanto_r_CreatePrimitive(fprim->vertexCount, fprim->indexCount, fprim->attrCount, fprim->attrSizes);
    const size_t indexDataSize  = fprim->indexCount * sizeof(Tanto_R_Index);
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        void* dst = tanto_r_GetPrimAttribute(&rprim, i);
        memcpy(dst, fprim->attributes[i], rprim.attrSizes[i] * rprim.vertexCount);
        memcpy(rprim.attrNames[i], fprim->attrNames[i], TANTO_R_ATTR_NAME_LEN);
    }
    memcpy(rprim.indexRegion.hostData, fprim->indices, indexDataSize);
    return rprim;
}

int tanto_f_WritePrimitive(const char* filename, const Tanto_F_Primitive* fprim)
{
    FILE* file = fopen(filename, "wb");
    assert(file);
    const size_t headerSize   = offsetof(Tanto_F_Primitive, attrSizes);
    assert(headerSize == 16);
    const size_t indexDataSize = sizeof(Tanto_R_Index) * fprim->indexCount;
    size_t r;
    r = fwrite(fprim, headerSize, 1, file);
    assert(r == 1);
    r = fwrite(fprim->attrSizes, sizeof(uint8_t) * fprim->attrCount, 1, file);
    assert(r == 1);
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        r = fwrite(fprim->attrNames[i], TANTO_R_ATTR_NAME_LEN, 1, file);
        assert(r == 1);
    }
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        r = fwrite(fprim->attributes[i], fprim->vertexCount * fprim->attrSizes[i], 1, file);
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
    const size_t headerSize   = offsetof(Tanto_F_Primitive, attrSizes);
    assert(headerSize == 16);
    r = fread(fprim, headerSize, 1, file);
    assert(r == 1);
    fprim->attrSizes  = malloc(fprim->attrCount * sizeof(Tanto_R_AttributeSize));
    fprim->indices    = malloc(fprim->indexCount * sizeof(Tanto_R_Index));
    fprim->attrNames  = malloc(fprim->attrCount * sizeof(char*));
    fprim->attributes = malloc(fprim->attrCount * sizeof(void*));
    r = fread(fprim->attrSizes, fprim->attrCount * sizeof(Tanto_R_AttributeSize), 1, file);
    assert(r);
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        fprim->attrNames[i] = malloc(TANTO_R_ATTR_NAME_LEN);
        r = fread(fprim->attrNames[i], TANTO_R_ATTR_NAME_LEN, 1, file);
        assert(r);
    }
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        fprim->attributes[i] = malloc(fprim->vertexCount * fprim->attrSizes[i]);
        r = fread(fprim->attributes[i], fprim->vertexCount * fprim->attrSizes[i], 1, file);
        assert(r);
    }
    fread(fprim->indices, fprim->indexCount * sizeof(Tanto_R_Index), 1, file);
    assert(r == 1);
    return 1;
}

void tanto_f_FreePrimitive(Tanto_F_Primitive* fprim)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    for (int i = 0; i < fprim->attrCount; i++) 
    {
        free(fprim->attributes[i]);
        free(fprim->attrNames[i]);
    }
    free(fprim->attributes);
    free(fprim->indices);
    free(fprim->attrNames);
    free(fprim->attrSizes);
    memset(fprim, 0, sizeof(Tanto_F_Primitive));
}
