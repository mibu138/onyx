#define COAL_SIMPLE_TYPE_NAMES
#include "geo.h"
#include "attribute.h"
#include "dtags.h"
#include "memory.h"
#include "mikktspace.h"
#include "private.h"
#include "render.h"
#include <hell/common.h>
#include <hell/debug.h>
#include <stdlib.h>
#include <string.h>

const int CW = 1;

typedef Onyx_GeoAttributeSize AttrSize;
typedef Onyx_Geometry         Prim;

typedef enum {
    ONYX_R_ATTRIBUTE_SFLOAT_TYPE,
} Onyx_R_AttributeType;

#define DPRINT(fmt, ...) hell_DebugPrint(ONYX_DEBUG_TAG_GEO, fmt, ##__VA_ARGS__)

// mikkt callbacks
static int
mikkt_GetNumFaces(const SMikkTSpaceContext* ctx)
{
    const Onyx_Geometry* geo = ctx->m_pUserData;
    return geo->indexCount / 3;
}

static int
mikkt_GetNumVerticesOfFace(const SMikkTSpaceContext* ctx, const int iFace)
{
    return 3;
}

static void
mikkt_GetPosition(const SMikkTSpaceContext* ctx, float fvPosOut[],
                  const int iFace, const int iVert)
{
    const Onyx_Geometry* geo = ctx->m_pUserData;
    int         index   = iFace * 3 + iVert;
    int  v = ((int*)geo->indexRegion.hostData)[index];

    Vec3* arr   = onyx_GetGeoAttribute2(geo, POS_NAME);
    fvPosOut[0] = arr[v].x;
    fvPosOut[1] = arr[v].y;
    fvPosOut[2] = arr[v].z;
}

static void
mikkt_GetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[],
                const int iFace, const int iVert)
{
    const Onyx_Geometry* geo = pContext->m_pUserData;
    int         index   = iFace * 3 + iVert;
    int  v = ((int*)geo->indexRegion.hostData)[index];

    Vec3* arr    = onyx_GetGeoAttribute2(geo, NORMAL_NAME);
    fvNormOut[0] = arr[v].x;
    fvNormOut[1] = arr[v].y;
    fvNormOut[2] = arr[v].z;
}

static void
mikkt_GetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[],
                  const int iFace, const int iVert)
{
    const Onyx_Geometry* geo = pContext->m_pUserData;
    int         index   = iFace * 3 + iVert;
    int  v = ((int*)geo->indexRegion.hostData)[index];

    Vec2* arr    = onyx_GetGeoAttribute2(geo, UV_NAME);
    fvTexcOut[0] = arr[v].x;
    fvTexcOut[1] = arr[v].y;
}

static void
mikkt_SetTSpaceBasic(const SMikkTSpaceContext* pContext,
                     const float fvTangent[], const float fSign,
                     const int iFace, const int iVert)
{
    Onyx_Geometry* geo = pContext->m_pUserData;
    int         index   = iFace * 3 + iVert;
    int  v = ((int*)geo->indexRegion.hostData)[index];

    Vec3* tangents = onyx_GetGeoAttribute2(geo, TANGENT_NAME);
    float* signs  = onyx_GetGeoAttribute2(geo, SIGN_NAME);

    tangents[v].x = fvTangent[0]; 
    tangents[v].y = fvTangent[1]; 
    tangents[v].z = fvTangent[2]; 
    signs[v] = fSign;
}

static void
initPrimBuffers(Onyx_Memory* memory, VkBufferUsageFlags extraFlags,
                Onyx_Geometry* prim)
{
    assert(prim->attrCount > 0);
    assert(prim->vertexCount > 0);
    assert(prim->attrCount < ONYX_R_MAX_VERT_ATTRIBUTES);

    size_t vertexBufferSize = 0;
    for (int i = 0; i < prim->attrCount; i++)
    {
        const AttrSize attrSize = prim->attrSizes[i];
        assert(attrSize > 0);
        const size_t attrRegionSize = prim->vertexCount * attrSize;
        prim->attrOffsets[i]        = vertexBufferSize;
        vertexBufferSize += attrRegionSize;
    }

    prim->vertexRegion =
        onyx_RequestBufferRegion(memory, vertexBufferSize,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | extraFlags,
                                 ONYX_MEMORY_HOST_GRAPHICS_TYPE);

    if (prim->indexCount > 0)
    {
        prim->indexRegion = onyx_RequestBufferRegion(
            memory, sizeof(Onyx_GeoIndex) * prim->indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | extraFlags,
            ONYX_MEMORY_HOST_GRAPHICS_TYPE);
    }
}

static void
printPrim(const Prim* prim)
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
        const size_t offset = prim->attrOffsets[i];
        hell_Print(">>> %s @ offset %ld: ", prim->attrNames[i], offset);
        for (int j = 0; j < prim->vertexCount; j++)
        {
            hell_Print("{");
            const int    dim = prim->attrSizes[i] / 4;
            const float* vals =
                &((float*)(prim->vertexRegion.hostData + offset))[j * dim];
            assert(vals != NULL);
            for (int k = 0; k < dim; k++)
                hell_Print("%f%s", vals[k], k == dim - 1 ? "" : ", ");
            hell_Print("%s", j == prim->vertexCount - 1 ? "}" : "}, ");
        }
        hell_Print("\n");
    }
    hell_Print("Indices: ");
    for (int i = 0; i < prim->indexCount; i++)
        hell_Print("%d%s", ((Onyx_GeoIndex*)prim->indexRegion.hostData)[i],
                   i == prim->indexCount - 1 ? "" : ", ");
    hell_Print("\n");
}

static VkFormat
getFormat(const Onyx_GeoAttributeSize attrSize,
          const Onyx_R_AttributeType  attrType)
{
    switch (attrType)
    {
    case ONYX_R_ATTRIBUTE_SFLOAT_TYPE:
        switch (attrSize)
        {
        case 4:
            return VK_FORMAT_R32_SFLOAT;
        case 8:
            return VK_FORMAT_R32G32_SFLOAT;
        case 12:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case 16:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            assert(0 && "Attribute size not supported");
        }
    default:
        assert(0 && "Attribute type not supported");
    }
}

void
onyx_PrintGeo(const Onyx_Geometry* prim)
{
    printPrim(prim);
}

void
onyx_TransferGeoToDevice(Onyx_Memory* memory, Onyx_Geometry* prim)
{
    onyx_TransferToDevice(memory, &prim->vertexRegion);
    if (prim->indexCount > 0)
    {
        onyx_TransferToDevice(memory, &prim->indexRegion);
    }
}

Onyx_Geometry
onyx_CreateTriangle(Onyx_Memory* memory)
{
    Onyx_Geometry prim = {.indexCount  = 3,
                          .vertexCount = 3,
                          .attrCount   = 2,
                          .attrSizes   = {12, 12}};

    initPrimBuffers(memory, 0x0, &prim);

    Vec3 positions[] = {
        {0.0, 0.5, 0.0},
        {0.5, -0.5, 0.0},
        {-0.5, -0.5, 0.0},
    };

    Vec3 colors[] = {{0.0, 0.9, 0.0}, {0.9, 0.5, 0.0}, {0.5, 0.3, 0.9}};

    Onyx_GeoIndex indices[] = {0, 1, 2};

    void*          posData   = onyx_GetGeoAttribute(&prim, 0);
    void*          colData   = onyx_GetGeoAttribute(&prim, 1);
    Onyx_GeoIndex* indexData = onyx_GetGeoIndices(&prim);

    memcpy(posData, positions, sizeof(positions));
    memcpy(colData, colors, sizeof(colors));
    memcpy(indexData, indices, sizeof(indices));

    return prim;
}

Onyx_Geometry
onyx_CreateCube(Onyx_Memory* memory, const bool isClockWise)
{
    const uint32_t vertCount  = 24;
    const uint32_t indexCount = 36;
    const uint32_t attrCount  = 3; // position, normals, uvw

    Onyx_Geometry prim = {.attrCount   = attrCount,
                          .indexCount  = indexCount,
                          .vertexCount = vertCount,
                          .attrSizes   = {12, 12, 8}};

    initPrimBuffers(memory, 0x0, &prim);

    const char attrNames[3][ONYX_R_ATTR_NAME_LEN] = {POS_NAME, NORMAL_NAME,
                                                     UV_NAME};
    for (int i = 0; i < attrCount; i++)
    {
        memcpy(prim.attrNames[i], attrNames[i], ONYX_R_ATTR_NAME_LEN);
    }

    Vec3* pPositions = onyx_GetGeoAttribute(&prim, 0);
    Vec3* pNormals   = onyx_GetGeoAttribute(&prim, 1);
    Vec2* pUvws      = onyx_GetGeoAttribute(&prim, 2);

    const Vec3 points[8] = {
        {-0.5, 0.5, 0.5},  {-0.5, -0.5, 0.5}, {0.5, -0.5, 0.5},
        {0.5, 0.5, 0.5},   {-0.5, 0.5, -0.5}, {-0.5, -0.5, -0.5},
        {0.5, -0.5, -0.5}, {0.5, 0.5, -0.5},
    };

    const Vec3 normals[6] = {
        {0.0, 0.0, 1.0},  {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0},
        {0.0, -1.0, 0.0}, {1.0, 0.0, 0.0},  {-1.0, 0.0, 0.0},
    };

    const Vec2 uvws[4] = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}};

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

    Onyx_GeoIndex* indices = onyx_GetGeoIndices(&prim);

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

    // onyx_v_TransferToDevice(&prim.vertexRegion);
    // onyx_v_TransferToDevice(&prim.indexRegion);

    return prim;
}

Onyx_Geometry
onyx_CreateCubeWithTangents(Onyx_Memory* memory, const bool isClockWise)
{
    const uint32_t vertCount  = 24;
    const uint32_t indexCount = 36;
    const uint32_t attrCount  = 5; // position, normals, tan, sign, uvw

    Onyx_Geometry geo = {.attrCount   = attrCount,
                         .indexCount  = indexCount,
                         .vertexCount = vertCount,
                         .attrSizes   = {12, 12, 12, 4, 8}};

    initPrimBuffers(memory, 0x0, &geo);

    const char attrNames[5][ONYX_R_ATTR_NAME_LEN] = {
        POS_NAME, NORMAL_NAME, TANGENT_NAME, SIGN_NAME, UV_NAME};
    for (int i = 0; i < attrCount; i++)
    {
        memcpy(geo.attrNames[i], attrNames[i], ONYX_R_ATTR_NAME_LEN);
    }

    Vec3* pPositions = onyx_GetGeoAttribute(&geo, 0);
    Vec3* pNormals   = onyx_GetGeoAttribute(&geo, 1);
    Vec2* pUvws      = onyx_GetGeoAttribute(&geo, 4);

    const Vec3 points[8] = {
        {-0.5, 0.5, 0.5},  {-0.5, -0.5, 0.5}, {0.5, -0.5, 0.5},
        {0.5, 0.5, 0.5},   {-0.5, 0.5, -0.5}, {-0.5, -0.5, -0.5},
        {0.5, -0.5, -0.5}, {0.5, 0.5, -0.5},
    };

    const Vec3 normals[6] = {
        {0.0, 0.0, 1.0},  {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0},
        {0.0, -1.0, 0.0}, {1.0, 0.0, 0.0},  {-1.0, 0.0, 0.0},
    };

    const Vec2 uvws[4] = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}};

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

    Onyx_GeoIndex* indices = onyx_GetGeoIndices(&geo);

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

    SMikkTSpaceInterface mikkt_interface = {
        .m_getNumFaces          = mikkt_GetNumFaces,
        .m_getNormal            = mikkt_GetNormal,
        .m_getNumVerticesOfFace = mikkt_GetNumVerticesOfFace,
        .m_getPosition          = mikkt_GetPosition,
        .m_getTexCoord          = mikkt_GetTexCoord,
        .m_setTSpaceBasic       = mikkt_SetTSpaceBasic,
    };

    SMikkTSpaceContext mikkt_context = {.m_pInterface = &mikkt_interface,
                                        .m_pUserData  = &geo};
    
    onyx_PrintGeo(&geo);
    tbool r = genTangSpaceDefault(&mikkt_context);
    assert(r);

    onyx_PrintGeo(&geo);

    // onyx_v_TransferToDevice(&prim.vertexRegion);
    // onyx_v_TransferToDevice(&prim.indexRegion);

    return geo;
}

Onyx_Geometry
onyx_CreatePoints(Onyx_Memory* memory, const uint32_t count)
{
    Onyx_Geometry prim = {
        .attrCount   = 2,
        .attrSizes   = {12, 12},
        .indexCount  = 0,
        .vertexCount = count,
    };

    prim.vertexRegion = onyx_RequestBufferRegion(
        memory, 12 * prim.attrCount * prim.vertexCount,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        ONYX_MEMORY_HOST_GRAPHICS_TYPE);

    const uint32_t posOffset = 0 * prim.vertexCount * 12;
    const uint32_t colOffset = 1 * prim.vertexCount * 12;

    prim.attrOffsets[0] = posOffset;
    prim.attrOffsets[1] = colOffset;

    Vec3* positions = onyx_GetGeoAttribute(&prim, 0);
    Vec3* colors    = onyx_GetGeoAttribute(&prim, 1);

    for (int i = 0; i < count; i++)
    {
        positions[i] = (Vec3){0, 0, 0};
        colors[i]    = (Vec3){1, 0, 0};
    }

    return prim;
}

Onyx_Geometry
onyx_CreateCurve(Onyx_Memory* memory, const uint32_t vertCount,
                 const uint32_t patchSize, const uint32_t restartOffset)
{
    assert(patchSize < vertCount);
    assert(restartOffset < patchSize);

    Onyx_Geometry prim = {
        .attrCount   = 2,
        .attrSizes   = {12, 12},
        .indexCount  = vertCount * patchSize, // to handle maximum restartOffset
        .vertexCount = vertCount,
    };

    initPrimBuffers(memory, 0x0, &prim);

    Vec3* positions = onyx_GetGeoAttribute(&prim, 0);
    Vec3* colors    = onyx_GetGeoAttribute(&prim, 1);

    for (int i = 0; i < vertCount; i++)
    {
        positions[i] = (Vec3){0, 0, 0};
        colors[i]    = (Vec3){1, 0, 0};
    }

    Onyx_GeoIndex* indices = onyx_GetGeoIndices(&prim);

    for (int i = 0, vertid = 0; vertid < prim.vertexCount;)
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

Onyx_Geometry
onyx_CreateQuadNDC(Onyx_Memory* memory, const float x, const float y,
                   const float width, const float height)
{
    Onyx_Geometry prim = {.attrCount   = 2,
                          .indexCount  = 6,
                          .vertexCount = 4,
                          .attrSizes   = {12, 12}};

    initPrimBuffers(memory, 0x0, &prim);

    Vec3* pos = onyx_GetGeoAttribute(&prim, 0);
    // upper left. x, y
    pos[0]    = (Vec3){x, y, 0};
    pos[1]    = (Vec3){x, y + height, 0};
    pos[2]    = (Vec3){x + width, y, 0};
    pos[3]    = (Vec3){x + width, y + height, 0};

    Vec3* uvw = onyx_GetGeoAttribute(&prim, 1);
    uvw[0]    = (Vec3){0, 0, 0};
    uvw[1]    = (Vec3){0, 1, 0};
    uvw[2]    = (Vec3){1, 0, 0};
    uvw[3]    = (Vec3){1, 1, 0};

    Onyx_GeoIndex* index = onyx_GetGeoIndices(&prim);
    index[0]             = 0;
    index[1]             = 1;
    index[2]             = 2;
    index[3]             = 2;
    index[4]             = 1;
    index[5]             = 3;

    return prim;
}

Onyx_Geometry
onyx_CreateQuadNDC_2(Onyx_Memory* memory, const float x, const float y,
                     const float width, const float height)
{
    Onyx_Geometry prim = {.attrCount   = 3,
                          .indexCount  = 6,
                          .vertexCount = 4,
                          .attrNames   = {POS_NAME, NORMAL_NAME, UV_NAME},
                          .attrSizes   = {12, 12, 8}};

    initPrimBuffers(memory, 0x0, &prim);

    Vec3* pos = onyx_GetGeoAttribute(&prim, 0);
    // upper left. x, y
    pos[0]    = (Vec3){x, y, 0};
    pos[1]    = (Vec3){x, y + height, 0};
    pos[2]    = (Vec3){x + width, y, 0};
    pos[3]    = (Vec3){x + width, y + height, 0};

    Vec3* n = onyx_GetGeoAttribute(&prim, 1);
    n[0]    = (Vec3){0, 0, 1};
    n[1]    = (Vec3){0, 0, 1};
    n[2]    = (Vec3){0, 0, 1};
    n[3]    = (Vec3){0, 0, 1};

    Vec2* uvs = onyx_GetGeoAttribute(&prim, 2);
    uvs[0]    = (Vec2){0, 0};
    uvs[1]    = (Vec2){0, 1};
    uvs[2]    = (Vec2){1, 0};
    uvs[3]    = (Vec2){1, 1};

    Onyx_GeoIndex* index = onyx_GetGeoIndices(&prim);
    index[0]             = 0;
    index[1]             = 1;
    index[2]             = 2;
    index[3]             = 2;
    index[4]             = 1;
    index[5]             = 3;

    return prim;
}

Onyx_Geometry
onyx_CreateGeometry(Onyx_Memory* memory, VkBufferUsageFlags extraBufferFlags,
                    const uint32_t vertCount, const uint32_t indexCount,
                    const uint8_t attrCount, const uint8_t attrSizes[])
{
    Onyx_Geometry prim = {.attrCount   = attrCount,
                          .indexCount  = indexCount,
                          .vertexCount = vertCount};

    assert(attrCount < ONYX_R_MAX_VERT_ATTRIBUTES);

    for (int i = 0; i < attrCount; i++)
    {
        prim.attrSizes[i] = attrSizes[i];
    }

    initPrimBuffers(memory, extraBufferFlags, &prim);

    return prim;
}

Onyx_VertexDescription
onyx_GetVertexDescription(const uint32_t              attrCount,
                          const Onyx_GeoAttributeSize attrSizes[])
{
    assert(attrCount < ONYX_R_MAX_VERT_ATTRIBUTES);
    Onyx_VertexDescription desc = {.attributeCount = attrCount,
                                   .bindingCount   = attrCount};

    for (int i = 0; i < desc.attributeCount; i++)
    {
        desc.bindingDescriptions[i] = (VkVertexInputBindingDescription){
            .binding   = i,
            .stride    = attrSizes[i],
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

        desc.attributeDescriptions[i] = (VkVertexInputAttributeDescription){
            .binding  = i,
            .location = i,
            .format   = getFormat(attrSizes[i], ONYX_R_ATTRIBUTE_SFLOAT_TYPE),
            .offset   = 0};
    }

    return desc;
}

void*
onyx_GetGeoAttribute(const Onyx_Geometry* prim, const uint32_t index)
{
    assert(index < ONYX_R_MAX_VERT_ATTRIBUTES);
    return (prim->vertexRegion.hostData + prim->attrOffsets[index]);
}

Onyx_GeoIndex*
onyx_GetGeoIndices(const Onyx_Geometry* prim)
{
    return (Onyx_GeoIndex*)prim->indexRegion.hostData;
}

void
onyx_BindGeo(const VkCommandBuffer cmdBuf, const Onyx_Geometry* prim)
{
    VkBuffer     vertBuffers[ONYX_R_MAX_VERT_ATTRIBUTES];
    VkDeviceSize attrOffsets[ONYX_R_MAX_VERT_ATTRIBUTES];

    for (int i = 0; i < prim->attrCount; i++)
    {
        vertBuffers[i] = prim->vertexRegion.buffer;
        attrOffsets[i] = prim->attrOffsets[i] + prim->vertexRegion.offset;
    }

    vkCmdBindVertexBuffers(cmdBuf, 0, prim->attrCount, vertBuffers,
                           attrOffsets);

    vkCmdBindIndexBuffer(cmdBuf, prim->indexRegion.buffer,
                         prim->indexRegion.offset, ONYX_VERT_INDEX_TYPE);
}

void
onyx_DrawGeo(const VkCommandBuffer cmdBuf, const Onyx_Geometry* prim)
{
    onyx_BindGeo(cmdBuf, prim);
    vkCmdDrawIndexed(cmdBuf, prim->indexCount, 1, 0, 0, 0);
}

void
onyx_FreeGeo(Onyx_Geometry* prim)
{
    onyx_FreeBufferRegion(&prim->vertexRegion);
    onyx_FreeBufferRegion(&prim->indexRegion);
}

VkDeviceSize
onyx_GetAttrOffset(const Onyx_Geometry* prim, const char* attrname)
{
    for (int i = 0; i < prim->attrCount; i++)
    {
        if (strncmp(prim->attrNames[i], attrname, ATTR_NAME_LEN) == 0)
            return prim->vertexRegion.offset + prim->attrOffsets[i];
    }
    assert(0 &&
           "No attribute by that name found, or prim contains no attributes");
    return 0;
}

VkDeviceSize
onyx_GetAttrOffset2(const Onyx_Geometry* prim, u32 index)
{
    return prim->vertexRegion.offset + prim->attrOffsets[index];
}

void*
onyx_GetGeoAttribute2(const Onyx_Geometry* prim, const char* name)
{
    int index = onyx_GetAttrIndex(prim, name);
    return onyx_GetGeoAttribute(prim, index);
}

int
onyx_GetAttrIndex(const Onyx_Geometry* prim, const char* attrname)
{
    for (int i = 0; i < prim->attrCount; i++)
    {
        if (strncmp(prim->attrNames[i], attrname, ATTR_NAME_LEN) == 0)
            return i;
    }
    assert(0 &&
           "No attribute by that name found, or prim contains no attributes");
    return 0;
}

VkDeviceSize
onyx_GetAttrRange(const Onyx_Geometry* prim, const char* attrname)
{
    for (int i = 0; i < prim->attrCount; i++)
    {
        if (strncmp(prim->attrNames[i], attrname, ATTR_NAME_LEN) == 0)
        {
            if (i < prim->attrCount - 1) // not the last attribute
                return prim->attrOffsets[i + 1] - prim->attrOffsets[i];
            else
                return prim->vertexRegion.size - prim->attrOffsets[i];
        }
    }
    assert(0 &&
           "No attribute by that name found, or prim contains no attributes");
    return 0;
}
