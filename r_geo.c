#include "r_geo.h"
#include "r_render.h"
#include "v_memory.h"
#include "t_def.h"
#include <string.h>
#include <stdlib.h>

const int CW = 1;

typedef Tanto_R_AttributeSize AttrSize;
typedef Tanto_R_Primitive     Prim;

static void initPrimBuffers(Tanto_R_Primitive* prim)
{
    assert(prim->attrCount > 0);
    assert(prim->vertexCount > 0);
    assert(prim->indexCount > 0);
    assert(prim->attrCount < TANTO_R_MAX_VERT_ATTRIBUTES);

    size_t vertexBufferSize = 0;
    for (int i = 0; i < prim->attrCount; i++) 
    {
        const AttrSize attrSize = prim->attrSizes[i];
        assert(attrSize > 0);
        const size_t attrRegionSize = prim->vertexCount * attrSize;
        prim->attrOffsets[i] = vertexBufferSize;
        vertexBufferSize += attrRegionSize;
    }

    prim->vertexRegion = tanto_v_RequestBufferRegion(vertexBufferSize, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, TANTO_V_MEMORY_HOST_GRAPHICS_TYPE);

    prim->indexRegion = tanto_v_RequestBufferRegion(sizeof(Tanto_R_Index) * prim->indexCount, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, TANTO_V_MEMORY_HOST_GRAPHICS_TYPE);
}

static void printPrim(const Prim* prim)
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
        const size_t offset = prim->attrOffsets[i];
        printf(">>> %s @ offset %ld: ", prim->attrNames[i], offset);
        for (int j = 0; j < prim->vertexCount; j++) 
        {
            printf("{");
            const int dim = prim->attrSizes[i]/4;
            const float* vals = &((float*)(prim->vertexRegion.hostData + offset))[j * dim];
            for (int k = 0; k < dim; k++)
                printf("%f%s", vals[k], k == dim - 1 ? "" : ", ");
            printf("%s", j == prim->vertexCount - 1 ? "}" : "}, ");
        }
        printf("\n");
    }
    printf("Indices: ");
    for (int i = 0; i < prim->indexCount; i++) 
        printf("%d%s", ((Tanto_R_Index*)prim->indexRegion.hostData)[i], i == prim->indexCount - 1 ? "" : ", ");
    printf("\n");
}

static VkFormat getFormat(const Tanto_R_AttributeSize attrSize, const Tanto_R_AttributeType attrType)
{
    switch (attrType)
    {
        case TANTO_R_ATTRIBUTE_SFLOAT_TYPE: switch (attrSize)
        {
            case  4: return VK_FORMAT_R32_SFLOAT;
            case  8: return VK_FORMAT_R32G32_SFLOAT;
            case 12: return VK_FORMAT_R32G32B32_SFLOAT;
            case 16: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: assert(0 && "Attribute size not supported");
        }
        default: assert(0 && "Attribute type not supported");
    }
}

void tanto_r_PrintPrim(const Tanto_R_Primitive* prim)
{
    printPrim(prim);
}

void tanto_r_TransferPrimToDevice(Tanto_R_Primitive* prim)
{
    tanto_v_TransferToDevice(&prim->vertexRegion);
    tanto_v_TransferToDevice(&prim->indexRegion);
}

Tanto_R_Primitive tanto_r_CreateTriangle(void)
{
    Tanto_R_Primitive prim = {
        .indexCount = 3, 
        .vertexCount = 3,
        .attrCount = 2,
        .attrSizes = {12, 12}
    };

    initPrimBuffers(&prim);

    Vec3 positions[] = {
        {0.0, 0.5, 0.0},
        {0.5, -0.5, 0.0},
        {-0.5, -0.5, 0.0},
    };

    Vec3 colors[] = {
        {0.0, 0.9, 0.0},
        {0.9, 0.5, 0.0},
        {0.5, 0.3, 0.9}
    };

    Tanto_R_Index indices[] = {0, 1, 2};

    void* posData = tanto_r_GetPrimAttribute(&prim, 0);
    void* colData = tanto_r_GetPrimAttribute(&prim, 1);
    Tanto_R_Index* indexData = tanto_r_GetPrimIndices(&prim);

    memcpy(posData, positions, sizeof(positions));
    memcpy(colData, colors, sizeof(colors));
    memcpy(indexData, indices, sizeof(indices));

    return prim;
}

Tanto_R_Primitive tanto_r_CreateCubePrim(const bool isClockWise)
{
    const uint32_t vertCount  = 24;
    const uint32_t indexCount = 36;
    const uint32_t attrCount  = 3; // position, normals, uvw

    Tanto_R_Primitive prim = {
        .attrCount = attrCount,
        .indexCount = indexCount,
        .vertexCount = vertCount,
        .attrSizes = {12, 12, 12}
    };

    initPrimBuffers(&prim);

    const char attrNames[3][TANTO_R_ATTR_NAME_LEN] = {"pos", "nor", "uvw"};
    for (int i = 0; i < attrCount; i++) 
    {
        memcpy(prim.attrNames[i], attrNames[i], TANTO_R_ATTR_NAME_LEN);
    }

    Vec3* pPositions = tanto_r_GetPrimAttribute(&prim, 0);
    Vec3* pNormals   = tanto_r_GetPrimAttribute(&prim, 1);
    Vec3* pUvws      = tanto_r_GetPrimAttribute(&prim, 2);

    const Vec3 points[8] = {
        { -0.5,  0.5,  0.5 },
        { -0.5, -0.5,  0.5 },
        {  0.5, -0.5,  0.5 },
        {  0.5,  0.5,  0.5 },
        { -0.5,  0.5, -0.5 },
        { -0.5, -0.5, -0.5 },
        {  0.5, -0.5, -0.5 },
        {  0.5,  0.5, -0.5 },
    };
    
    const Vec3 normals[6] = {
        {  0.0,  0.0,  1.0 },
        {  0.0,  0.0, -1.0 },
        {  0.0,  1.0,  0.0 },
        {  0.0, -1.0,  0.0 },
        {  1.0,  0.0,  0.0 },
        { -1.0,  0.0,  0.0 },
    };

    const Vec3 uvws[4] = {
        { 0.0, 0.0, 0.0 },
        { 1.0, 0.0, 0.0 },
        { 0.0, 1.0, 0.0 },
        { 1.0, 1.0, 0.0 }
    };

    // face 0: +Z
    pPositions[0]  = points[0];
    pPositions[1]  = points[1];
    pPositions[2]  = points[2];
    pPositions[3]  = points[3];
    // face 1: -Z
    pPositions[4]  = points[7];
    pPositions[5]  = points[6];
    pPositions[6]  = points[5];
    pPositions[7]  = points[4];
    // face 2: -X
    pPositions[8]  = points[0];
    pPositions[9]  = points[4];
    pPositions[10] = points[5];
    pPositions[11] = points[1];
    // face 3: +X
    pPositions[12] = points[3];
    pPositions[13] = points[2];
    pPositions[14] = points[6];
    pPositions[15] = points[7];
    // face 4: +Y
    pPositions[16] = points[0];
    pPositions[17] = points[3];
    pPositions[18] = points[7];
    pPositions[19] = points[4];
    // face 5: -Y
    pPositions[20] = points[2];
    pPositions[21] = points[1];
    pPositions[22] = points[5];
    pPositions[23] = points[6];

    pNormals[0]  = normals[0];
    pNormals[1]  = normals[0];
    pNormals[2]  = normals[0];
    pNormals[3]  = normals[0];
    // pNormals -Z
    pNormals[4]  = normals[1];
    pNormals[5]  = normals[1];
    pNormals[6]  = normals[1];
    pNormals[7]  = normals[1];
    // pNormals -X
    pNormals[8]  = normals[5];
    pNormals[9]  = normals[5];
    pNormals[10] = normals[5];
    pNormals[11] = normals[5];
    // pNormals +X
    pNormals[12] = normals[4];
    pNormals[13] = normals[4];
    pNormals[14] = normals[4];
    pNormals[15] = normals[4];
    // pNormals +Y
    pNormals[16] = normals[2];
    pNormals[17] = normals[2];
    pNormals[18] = normals[2];
    pNormals[19] = normals[2];
    // pNormals -Y
    pNormals[20] = normals[3];
    pNormals[21] = normals[3];
    pNormals[22] = normals[3];
    pNormals[23] = normals[3];

    for (int i = 0; i < vertCount; i += 4) 
    {
        pUvws[i + 1] = uvws[0];
        pUvws[i + 0] = uvws[1];
        pUvws[i + 2] = uvws[2];
        pUvws[i + 3] = uvws[3];
    }

    Tanto_R_Index* indices = tanto_r_GetPrimIndices(&prim);

    if (isClockWise)
        for (int face = 0; face < indexCount / 6; face++) 
        {
            indices[face * 6 + 2] = 4 * face + 0;
            indices[face * 6 + 1] = 4 * face + 1;
            indices[face * 6 + 0] = 4 * face + 2;
            indices[face * 6 + 5] = 4 * face + 0;
            indices[face * 6 + 4] = 4 * face + 2;
            indices[face * 6 + 3] = 4 * face + 3;
        }
    else
        for (int face = 0; face < indexCount / 6; face++) 
        {
            indices[face * 6 + 0] = 4 * face + 0;
            indices[face * 6 + 1] = 4 * face + 1;
            indices[face * 6 + 2] = 4 * face + 2;
            indices[face * 6 + 3] = 4 * face + 0;
            indices[face * 6 + 4] = 4 * face + 2;
            indices[face * 6 + 5] = 4 * face + 3;
        }

    //tanto_v_TransferToDevice(&prim.vertexRegion);
    //tanto_v_TransferToDevice(&prim.indexRegion);

    return prim;
}

Tanto_R_Primitive tanto_r_CreateCubePrimUV(const bool isClockWise)
{
    const uint32_t vertCount  = 24;
    const uint32_t indexCount = 36;
    const uint32_t attrCount  = 3; // position, normals, uvw

    Tanto_R_Primitive prim = {
        .attrCount = attrCount,
        .indexCount = indexCount,
        .vertexCount = vertCount,
        .attrSizes = {12, 12, 8}
    };

    initPrimBuffers(&prim);

    const char attrNames[3][TANTO_R_ATTR_NAME_LEN] = {"pos", "nor", "uv"};
    for (int i = 0; i < attrCount; i++) 
    {
        memcpy(prim.attrNames[i], attrNames[i], TANTO_R_ATTR_NAME_LEN);
    }

    Vec3* pPositions = tanto_r_GetPrimAttribute(&prim, 0);
    Vec3* pNormals   = tanto_r_GetPrimAttribute(&prim, 1);
    Vec2* pUvws      = tanto_r_GetPrimAttribute(&prim, 2);

    const Vec3 points[8] = {
        { -0.5,  0.5,  0.5 },
        { -0.5, -0.5,  0.5 },
        {  0.5, -0.5,  0.5 },
        {  0.5,  0.5,  0.5 },
        { -0.5,  0.5, -0.5 },
        { -0.5, -0.5, -0.5 },
        {  0.5, -0.5, -0.5 },
        {  0.5,  0.5, -0.5 },
    };
    
    const Vec3 normals[6] = {
        {  0.0,  0.0,  1.0 },
        {  0.0,  0.0, -1.0 },
        {  0.0,  1.0,  0.0 },
        {  0.0, -1.0,  0.0 },
        {  1.0,  0.0,  0.0 },
        { -1.0,  0.0,  0.0 },
    };

    const Vec2 uvws[4] = {
        { 0.0, 0.0 },
        { 1.0, 0.0 },
        { 0.0, 1.0 },
        { 1.0, 1.0 }
    };

    // face 0: +Z
    pPositions[0]  = points[0];
    pPositions[1]  = points[1];
    pPositions[2]  = points[2];
    pPositions[3]  = points[3];
    // face 1: -Z
    pPositions[4]  = points[7];
    pPositions[5]  = points[6];
    pPositions[6]  = points[5];
    pPositions[7]  = points[4];
    // face 2: -X
    pPositions[8]  = points[0];
    pPositions[9]  = points[4];
    pPositions[10] = points[5];
    pPositions[11] = points[1];
    // face 3: +X
    pPositions[12] = points[3];
    pPositions[13] = points[2];
    pPositions[14] = points[6];
    pPositions[15] = points[7];
    // face 4: +Y
    pPositions[16] = points[0];
    pPositions[17] = points[3];
    pPositions[18] = points[7];
    pPositions[19] = points[4];
    // face 5: -Y
    pPositions[20] = points[2];
    pPositions[21] = points[1];
    pPositions[22] = points[5];
    pPositions[23] = points[6];

    pNormals[0]  = normals[0];
    pNormals[1]  = normals[0];
    pNormals[2]  = normals[0];
    pNormals[3]  = normals[0];
    // pNormals -Z
    pNormals[4]  = normals[1];
    pNormals[5]  = normals[1];
    pNormals[6]  = normals[1];
    pNormals[7]  = normals[1];
    // pNormals -X
    pNormals[8]  = normals[5];
    pNormals[9]  = normals[5];
    pNormals[10] = normals[5];
    pNormals[11] = normals[5];
    // pNormals +X
    pNormals[12] = normals[4];
    pNormals[13] = normals[4];
    pNormals[14] = normals[4];
    pNormals[15] = normals[4];
    // pNormals +Y
    pNormals[16] = normals[2];
    pNormals[17] = normals[2];
    pNormals[18] = normals[2];
    pNormals[19] = normals[2];
    // pNormals -Y
    pNormals[20] = normals[3];
    pNormals[21] = normals[3];
    pNormals[22] = normals[3];
    pNormals[23] = normals[3];

    for (int i = 0; i < vertCount; i += 4) 
    {
        pUvws[i + 1] = uvws[0];
        pUvws[i + 0] = uvws[1];
        pUvws[i + 2] = uvws[2];
        pUvws[i + 3] = uvws[3];
    }

    Tanto_R_Index* indices = tanto_r_GetPrimIndices(&prim);

    if (isClockWise)
        for (int face = 0; face < indexCount / 6; face++) 
        {
            indices[face * 6 + 2] = 4 * face + 0;
            indices[face * 6 + 1] = 4 * face + 1;
            indices[face * 6 + 0] = 4 * face + 2;
            indices[face * 6 + 5] = 4 * face + 0;
            indices[face * 6 + 4] = 4 * face + 2;
            indices[face * 6 + 3] = 4 * face + 3;
        }
    else
        for (int face = 0; face < indexCount / 6; face++) 
        {
            indices[face * 6 + 0] = 4 * face + 0;
            indices[face * 6 + 1] = 4 * face + 1;
            indices[face * 6 + 2] = 4 * face + 2;
            indices[face * 6 + 3] = 4 * face + 0;
            indices[face * 6 + 4] = 4 * face + 2;
            indices[face * 6 + 5] = 4 * face + 3;
        }

    //tanto_v_TransferToDevice(&prim.vertexRegion);
    //tanto_v_TransferToDevice(&prim.indexRegion);

    return prim;
}

Tanto_R_Primitive tanto_r_CreatePoints(const uint32_t count)
{
    Tanto_R_Primitive prim = {
        .attrCount = 2,
        .attrSizes = {12, 12},
        .indexCount = 0, 
        .vertexCount = count,
    };

    prim.vertexRegion = tanto_v_RequestBufferRegion(
            12 * prim.attrCount * prim.vertexCount, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
            TANTO_V_MEMORY_HOST_GRAPHICS_TYPE);

    const uint32_t posOffset = 0 * prim.vertexCount * 12;
    const uint32_t colOffset = 1 * prim.vertexCount * 12;

    prim.attrOffsets[0] = posOffset;
    prim.attrOffsets[1] = colOffset;

    Vec3* positions = tanto_r_GetPrimAttribute(&prim, 0);
    Vec3* colors    = tanto_r_GetPrimAttribute(&prim, 1);

    for (int i = 0; i < count; i++) 
    {
        positions[i] = (Vec3){0, 0, 0};  
        colors[i] = (Vec3){1, 0, 0};  
    }

    return prim;
}

Tanto_R_Primitive tanto_r_CreateCurve(const uint32_t vertCount, const uint32_t patchSize, const uint32_t restartOffset)
{
    assert(patchSize < vertCount);
    assert(restartOffset < patchSize);

    Tanto_R_Primitive prim = {
        .attrCount = 2,
        .attrSizes = {12, 12},
        .indexCount = vertCount * patchSize, // to handle maximum restartOffset
        .vertexCount = vertCount,
    };

    initPrimBuffers(&prim);

    Vec3* positions = tanto_r_GetPrimAttribute(&prim, 0);
    Vec3* colors    = tanto_r_GetPrimAttribute(&prim, 1);

    for (int i = 0; i < vertCount; i++) 
    {
        positions[i] = (Vec3){0, 0, 0};  
        colors[i] = (Vec3){1, 0, 0};  
    }

    Tanto_R_Index* indices = tanto_r_GetPrimIndices(&prim);

    for (int i = 0, vertid = 0; vertid < prim.vertexCount; ) 
    {
        assert(i < prim.indexCount);
        assert(vertid <= i);
        indices[i] = vertid++;
        i++;
        if (i % patchSize == 0)
            vertid -= restartOffset;
        assert(vertid >= 0);
    }

    return prim;
}

Tanto_R_Primitive tanto_r_CreateQuad(const float width, const float height, Tanto_R_AttributeBits attribBits)
{
    const int attrCount = __builtin_popcount(attribBits) + 1;

    Tanto_R_Primitive prim = {
        .attrCount = attrCount,
        .indexCount = 6,
        .vertexCount = 4,
    };

    for (int i = 0; i < attrCount; i++) 
    {
        prim.attrSizes[i] = 12;
    }

    initPrimBuffers(&prim);

    Vec3* pos = tanto_r_GetPrimAttribute(&prim, 0);

    const float w = width / 2;
    const float h = height / 2;

    pos[0] = (Vec3){-w,  h, 0};
    pos[1] = (Vec3){-w, -h, 0};
    pos[2] = (Vec3){ w,  h, 0};
    pos[3] = (Vec3){ w, -h, 0};

    for (int i = 1; i < attrCount; i++) 
    {
        if (attribBits & TANTO_R_ATTRIBUTE_NORMAL_BIT)
        {
            Vec3* normals = tanto_r_GetPrimAttribute(&prim, i);
            normals[0] = (Vec3){0, 0, 1};
            normals[1] = (Vec3){0, 0, 1};
            normals[2] = (Vec3){0, 0, 1};
            normals[3] = (Vec3){0, 0, 1};
            attribBits &= ~TANTO_R_ATTRIBUTE_NORMAL_BIT;
        }
        else if (attribBits & TANTO_R_ATTRIBUTE_UVW_BIT)
        {
            Vec3* uvw = tanto_r_GetPrimAttribute(&prim, i);
            uvw[0] = (Vec3){0, 0, 0};
            uvw[1] = (Vec3){0, 1, 0};
            uvw[2] = (Vec3){1, 0, 0};
            uvw[3] = (Vec3){1, 1, 0};
            attribBits &= ~TANTO_R_ATTRIBUTE_UVW_BIT;
        }
    }

    Tanto_R_Index* index = tanto_r_GetPrimIndices(&prim);

    index[0] = 0;
    index[1] = 1;
    index[2] = 2;
    index[3] = 2;
    index[4] = 1;
    index[5] = 3;

    tanto_v_TransferToDevice(&prim.vertexRegion);
    tanto_v_TransferToDevice(&prim.indexRegion);

    return prim;
}

Tanto_R_Primitive tanto_r_CreateQuadNDC(const float x, const float y, const float width, const float height)
{
    Tanto_R_Primitive prim = {
        .attrCount = 2,
        .indexCount = 6,
        .vertexCount = 4,
        .attrSizes = {12, 12}
    };

    initPrimBuffers(&prim);

    Vec3* pos = tanto_r_GetPrimAttribute(&prim, 0);
    // upper left. x, y
    pos[0] = (Vec3){x, y, 0};
    pos[1] = (Vec3){x, y + height, 0};
    pos[2] = (Vec3){x + width, y, 0};
    pos[3] = (Vec3){x + width, y + height, 0};

    Vec3* uvw = tanto_r_GetPrimAttribute(&prim, 1);
    uvw[0] = (Vec3){0, 0, 0};
    uvw[1] = (Vec3){0, 1, 0};
    uvw[2] = (Vec3){1, 0, 0};
    uvw[3] = (Vec3){1, 1, 0};

    Tanto_R_Index* index = tanto_r_GetPrimIndices(&prim);
    index[0] = 0;
    index[1] = 1;
    index[2] = 2;
    index[3] = 2;
    index[4] = 1;
    index[5] = 3;

    return prim;
}

Tanto_R_Primitive tanto_r_CreatePrimitive(const uint32_t vertCount, const uint32_t indexCount, 
        const uint8_t attrCount, const uint8_t attrSizes[attrCount])
{
    Tanto_R_Primitive prim = {
        .attrCount = attrCount,
        .indexCount = indexCount,
        .vertexCount = vertCount
    };

    assert(attrCount < TANTO_R_MAX_VERT_ATTRIBUTES);

    for (int i = 0; i < attrCount; i++) 
    {
        prim.attrSizes[i] = attrSizes[i];
    }

    initPrimBuffers(&prim);

    return prim;
}

Tanto_R_VertexDescription tanto_r_GetVertexDescription(const uint32_t attrCount, const Tanto_R_AttributeSize attrSizes[attrCount])
{
    assert(attrCount < TANTO_R_MAX_VERT_ATTRIBUTES);
    Tanto_R_VertexDescription desc = {
        .attributeCount = attrCount,
        .bindingCount   = attrCount
    };

    for (int i = 0; i < desc.attributeCount; i++) 
    {
        desc.bindingDescriptions[i] = (VkVertexInputBindingDescription){
            .binding   = i,
            .stride    = attrSizes[i],
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        desc.attributeDescriptions[i] = (VkVertexInputAttributeDescription){
            .binding  = i,
            .location = i,
            .format   = getFormat(attrSizes[i], TANTO_R_ATTRIBUTE_SFLOAT_TYPE),
            .offset   = 0
        };
    }

    return desc;
}

void* tanto_r_GetPrimAttribute(const Tanto_R_Primitive* prim, const uint32_t index)
{
    assert(index < TANTO_R_MAX_VERT_ATTRIBUTES);
    return (prim->vertexRegion.hostData + prim->attrOffsets[index]);
}

Tanto_R_Index* tanto_r_GetPrimIndices(const Tanto_R_Primitive* prim)
{
    return (Tanto_R_Index*)prim->indexRegion.hostData;
}

void tanto_r_BindPrim(const VkCommandBuffer cmdBuf, const Tanto_R_Primitive* prim)
{
    VkBuffer     vertBuffers[prim->attrCount];
    VkDeviceSize attrOffsets[prim->attrCount];

    for (int i = 0; i < prim->attrCount; i++) 
    {
        vertBuffers[i] = prim->vertexRegion.buffer;
        attrOffsets[i] = prim->attrOffsets[i] + prim->vertexRegion.offset;
    }

    vkCmdBindVertexBuffers(cmdBuf, 0, prim->attrCount, vertBuffers, attrOffsets);

    vkCmdBindIndexBuffer(cmdBuf, prim->indexRegion.buffer, 
            prim->indexRegion.offset, TANTO_VERT_INDEX_TYPE);
}

void tanto_r_DrawPrim(const VkCommandBuffer cmdBuf, const Tanto_R_Primitive* prim)
{
    tanto_r_BindPrim(cmdBuf, prim);
    vkCmdDrawIndexed(cmdBuf, prim->indexCount, 1, 0, 0, 0);
}

void tanto_r_FreePrim(Tanto_R_Primitive *prim)
{
    tanto_v_FreeBufferRegion(&prim->vertexRegion);
    tanto_v_FreeBufferRegion(&prim->indexRegion);
}

