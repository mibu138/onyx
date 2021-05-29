#include "f_file.h"
#include "r_geo.h"
#include "v_memory.h"
#include <hell/attributes.h>
#include <hell/common.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(uint8_t) == sizeof(Obdn_R_AttributeSize),
               "sizeof(Obdn_R_AttributeSize) must be 1");

typedef Obdn_F_Primitive FPrim;

#define FCHECK

static void
printPrim(const FPrim* prim)
{
    hell_Print(">>> printing Fprim info...\n");
    hell_Print(">>> attrCount %d vertexCount %d indexCount %d \n",
               prim->attrCount, prim->vertexCount, prim->indexCount);
    hell_Print(">>> attrSizes ");
    for (int i = 0; i < prim->attrCount; i++)
        hell_Print("%d ", prim->attrSizes[i]);
    hell_Print("\n>>> attrNames ");
    for (int i = 0; i < prim->attrCount; i++)
        hell_Print("%s ", prim->attrNames[i]);
    hell_Print("\n>>> Attributes: \n");
    for (int i = 0; i < prim->attrCount; i++)
    {
        hell_Print(">>> %s: ", prim->attrNames[i]);
        for (int j = 0; j < prim->vertexCount; j++)
        {
            hell_Print("{");
            const int    dim  = prim->attrSizes[i] / 4;
            const float* vals = &((float**)prim->attributes)[i][j * dim];
            for (int k = 0; k < dim; k++)
                hell_Print("%f%s", vals[k], k == dim - 1 ? "" : ", ");
            hell_Print("%s", j == prim->vertexCount - 1 ? "}" : "}, ");
        }
        hell_Print("\n");
    }
    hell_Print("Indices: ");
    for (int i = 0; i < prim->indexCount; i++)
        hell_Print("%d%s", prim->indices[i],
                   i == prim->indexCount - 1 ? "" : ", ");
    hell_Print("\n");
}

void
obdn_f_PrintPrim(const Obdn_F_Primitive* prim)
{
    printPrim(prim);
}

Obdn_F_Primitive
obdn_f_CreatePrimitive(const uint32_t vertexCount, const uint32_t indexCount,
                       const uint32_t             attrCount,
                       const Obdn_R_AttributeSize attrSizes[attrCount],
                       const char attrNames[attrCount][OBDN_R_ATTR_NAME_LEN])
{
    Obdn_F_Primitive fprim = {.vertexCount = vertexCount,
                              .indexCount  = indexCount,
                              .attrCount   = attrCount};

    fprim.attrSizes  = hell_Malloc(attrCount * sizeof(Obdn_R_AttributeSize));
    fprim.indices    = hell_Malloc(indexCount * sizeof(Obdn_R_Index));
    fprim.attrNames  = hell_Malloc(attrCount * sizeof(char*));
    fprim.attributes = hell_Malloc(attrCount * sizeof(void*));

    for (int i = 0; i < attrCount; i++)
    {
        fprim.attrSizes[i]  = attrSizes[i];
        fprim.attrNames[i]  = hell_Malloc(OBDN_R_ATTR_NAME_LEN);
        fprim.attributes[i] = hell_Malloc(vertexCount * attrSizes[i]);
    }

    if (attrNames != NULL)
    {
        for (int i = 0; i < attrCount; i++)
        {
            memcpy(fprim.attrNames[i], attrNames[i], OBDN_R_ATTR_NAME_LEN);
        }
    }

    return fprim;
}

Obdn_F_Primitive
obdn_f_CreateFPrimFromRPrim(Obdn_Memory* memory, const Obdn_R_Primitive* rprim)
{
    Obdn_F_Primitive fprim = obdn_f_CreatePrimitive(
        rprim->vertexCount, rprim->indexCount, rprim->attrCount,
        rprim->attrSizes, rprim->attrNames);

    Obdn_V_BufferRegion hostVertRegion;
    Obdn_V_BufferRegion hostIndexRegion;

    size_t attrDataSize = 0;
    for (int i = 0; i < rprim->attrCount; i++)
    {
        attrDataSize += rprim->vertexCount * rprim->attrSizes[i];
    }

    assert(attrDataSize > 0);

    if (!rprim->vertexRegion.hostData)
    {
        // must copy data to host
        hostVertRegion = obdn_RequestBufferRegion(
            memory, attrDataSize, 0, OBDN_V_MEMORY_HOST_GRAPHICS_TYPE);
        hostIndexRegion = obdn_RequestBufferRegion(
            memory, rprim->indexCount * sizeof(Obdn_R_Index), 0,
            OBDN_V_MEMORY_HOST_GRAPHICS_TYPE);
        obdn_CopyBufferRegion(&rprim->vertexRegion, &hostVertRegion);
        obdn_CopyBufferRegion(&rprim->indexRegion, &hostIndexRegion);
    }
    else
    {
        hostVertRegion  = rprim->vertexRegion;
        hostIndexRegion = rprim->indexRegion;
    }

    for (int i = 0; i < rprim->attrCount; i++)
    {
        const size_t chunkSize = rprim->attrSizes[i] * rprim->vertexCount;
        const void*  src = hostVertRegion.hostData + rprim->attrOffsets[i];
        memcpy(fprim.attributes[i], src, chunkSize);
        // src = rprim->attrNames[i];
        // memcpy(fprim.attrNames[i], src, OBDN_R_ATTR_NAME_LEN);
        fprim.attrSizes[i] = rprim->attrSizes[i];
    }

    memcpy(fprim.indices, hostIndexRegion.hostData,
           rprim->indexCount * sizeof(Obdn_R_Index));

    if (!rprim->vertexRegion.hostData)
    {
        obdn_FreeBufferRegion(&hostVertRegion);
        obdn_FreeBufferRegion(&hostIndexRegion);
    }

    return fprim;
}

Obdn_R_Primitive
obdn_f_CreateRPrimFromFPrim(Obdn_Memory* memory, const Obdn_F_Primitive* fprim)
{
    Obdn_R_Primitive rprim =
        obdn_CreatePrimitive(memory, fprim->vertexCount, fprim->indexCount,
                             fprim->attrCount, fprim->attrSizes);
    const size_t indexDataSize = fprim->indexCount * sizeof(Obdn_R_Index);
    for (int i = 0; i < fprim->attrCount; i++)
    {
        void* dst = obdn_r_GetPrimAttribute(&rprim, i);
        memcpy(dst, fprim->attributes[i],
               rprim.attrSizes[i] * rprim.vertexCount);
        memcpy(rprim.attrNames[i], fprim->attrNames[i], OBDN_R_ATTR_NAME_LEN);
    }
    memcpy(rprim.indexRegion.hostData, fprim->indices, indexDataSize);
    return rprim;
}

int
obdn_f_WritePrimitive(const char* filename, const Obdn_F_Primitive* fprim)
{
    FILE* file = fopen(filename, "wb");
    assert(file);
    const size_t headerSize = offsetof(Obdn_F_Primitive, attrSizes);
    assert(headerSize == 16);
    const size_t indexDataSize = sizeof(Obdn_R_Index) * fprim->indexCount;
    size_t       r;
    r = fwrite(fprim, headerSize, 1, file);
    assert(r == 1);
    r = fwrite(fprim->attrSizes, sizeof(uint8_t) * fprim->attrCount, 1, file);
    assert(r == 1);
    for (int i = 0; i < fprim->attrCount; i++)
    {
        r = fwrite(fprim->attrNames[i], OBDN_R_ATTR_NAME_LEN, 1, file);
        assert(r == 1);
    }
    for (int i = 0; i < fprim->attrCount; i++)
    {
        r = fwrite(fprim->attributes[i],
                   fprim->vertexCount * fprim->attrSizes[i], 1, file);
        assert(r == 1);
    }
    r = fwrite(fprim->indices, indexDataSize, 1, file);
    assert(r == 1);
    r = fclose(file);
    assert(r == 0);
    return 1;
}

int
obdn_f_ReadPrimitive(const char* filename, Obdn_F_Primitive* fprim)
{
    FILE* file = fopen(filename, "rb");
    assert(file);
    size_t r     UNUSED;
    const size_t headerSize = offsetof(Obdn_F_Primitive, attrSizes);
    assert(headerSize == 16);
    r = fread(fprim, headerSize, 1, file);
    assert(r == 1);
    fprim->attrSizes =
        hell_Malloc(fprim->attrCount * sizeof(Obdn_R_AttributeSize));
    fprim->indices    = hell_Malloc(fprim->indexCount * sizeof(Obdn_R_Index));
    fprim->attrNames  = hell_Malloc(fprim->attrCount * sizeof(char*));
    fprim->attributes = hell_Malloc(fprim->attrCount * sizeof(void*));
    r = fread(fprim->attrSizes, fprim->attrCount * sizeof(Obdn_R_AttributeSize),
              1, file);
    assert(r);
    for (int i = 0; i < fprim->attrCount; i++)
    {
        fprim->attrNames[i] = hell_Malloc(OBDN_R_ATTR_NAME_LEN);
        r = fread(fprim->attrNames[i], OBDN_R_ATTR_NAME_LEN, 1, file);
        assert(r);
    }
    for (int i = 0; i < fprim->attrCount; i++)
    {
        fprim->attributes[i] =
            hell_Malloc(fprim->vertexCount * fprim->attrSizes[i]);
        r = fread(fprim->attributes[i],
                  fprim->vertexCount * fprim->attrSizes[i], 1, file);
        assert(r);
    }
    fread(fprim->indices, fprim->indexCount * sizeof(Obdn_R_Index), 1, file);
    assert(r == 1);
    return 1;
}

Obdn_R_Primitive
obdn_f_LoadRPrim(Obdn_Memory* memory, const char* filename,
                 const bool transferToDevice)
{
    Obdn_F_Primitive fprim;
    int              r;
    r = obdn_f_ReadPrimitive(filename, &fprim);
    assert(r);
    Obdn_R_Primitive rprim = obdn_f_CreateRPrimFromFPrim(memory, &fprim);
    obdn_f_FreePrimitive(&fprim);
    if (transferToDevice)
    {
        obdn_TransferPrimToDevice(memory, &rprim);
    }
    return rprim;
}

void
obdn_f_FreePrimitive(Obdn_F_Primitive* fprim)
{
    for (int i = 0; i < fprim->attrCount; i++)
    {
        hell_Free(fprim->attributes[i]);
        hell_Free(fprim->attrNames[i]);
    }
    hell_Free(fprim->attributes);
    hell_Free(fprim->indices);
    hell_Free(fprim->attrNames);
    hell_Free(fprim->attrSizes);
    memset(fprim, 0, sizeof(Obdn_F_Primitive));
}
