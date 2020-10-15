#include "r_geo.h"
#include <string.h>

const int CW = 1;

Tanto_R_Mesh tanto_r_PreMeshToMesh(const Tanto_R_PreMesh pm)
{
    Tanto_R_Mesh m = {};
    size_t nverts = pm.vertexCount;
    m.vertexCount = nverts;
    m.vertexBlock = tanto_v_RequestBufferRegion(nverts * sizeof(Tanto_R_Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    m.posOffset   = nverts * sizeof(Tanto_R_Attribute) * 0;
    m.colOffset   = nverts * sizeof(Tanto_R_Attribute) * 1;
    m.norOffset   = nverts * sizeof(Tanto_R_Attribute) * 2;
    m.uvwOffset   = nverts * sizeof(Tanto_R_Attribute) * 3;
    m.indexBlock  = tanto_v_RequestBufferRegion(nverts * sizeof(Tanto_R_Index), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    m.indexCount  = nverts;

    memcpy(m.vertexBlock.hostData + m.posOffset, pm.posData, sizeof(Tanto_R_Attribute) * nverts);
    memcpy(m.vertexBlock.hostData + m.colOffset, pm.colData, sizeof(Tanto_R_Attribute) * nverts);
    memcpy(m.vertexBlock.hostData + m.norOffset, pm.norData, sizeof(Tanto_R_Attribute) * nverts);
    memcpy(m.vertexBlock.hostData + m.uvwOffset, pm.uvwData, sizeof(Tanto_R_Attribute) * nverts);
    memcpy(m.indexBlock.hostData, pm.indexData, sizeof(Tanto_R_Index) * nverts);
    
    return m;
}

Tanto_R_Mesh tanto_r_CreateMesh(uint32_t vertexCount, uint32_t indexCount)
{
    Tanto_R_Mesh mesh;
    mesh.vertexCount = vertexCount;
    mesh.indexCount  = indexCount;
    mesh.vertexBlock = tanto_v_RequestBufferRegion(sizeof(Tanto_R_Vertex) * mesh.vertexCount, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    mesh.indexBlock  = tanto_v_RequestBufferRegion(sizeof(Tanto_R_Index) * mesh.indexCount, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    mesh.posOffset = 0 * mesh.vertexCount * sizeof(Tanto_R_Attribute);
    mesh.colOffset = 1 * mesh.vertexCount * sizeof(Tanto_R_Attribute);
    mesh.norOffset = 2 * mesh.vertexCount * sizeof(Tanto_R_Attribute);
    mesh.uvwOffset = 3 * mesh.vertexCount * sizeof(Tanto_R_Attribute);
    return mesh;
}

Tanto_R_Mesh tanto_r_CreateCube(void)
{
    const uint32_t vertCount  = 24;
    const uint32_t indexCount = 36;
    Tanto_R_Mesh mesh = tanto_r_CreateMesh(vertCount, indexCount);
    Tanto_R_Attribute* pPositions = (Tanto_R_Attribute*)(mesh.vertexBlock.hostData + mesh.posOffset);
    Tanto_R_Attribute* pColors    = (Tanto_R_Attribute*)(mesh.vertexBlock.hostData + mesh.colOffset);
    Tanto_R_Attribute* pNormals   = (Tanto_R_Attribute*)(mesh.vertexBlock.hostData + mesh.norOffset);
    Tanto_R_Attribute* pUvws      = (Tanto_R_Attribute*)(mesh.vertexBlock.hostData + mesh.uvwOffset);
    Tanto_R_Index*  indices       = (Tanto_R_Index*)mesh.indexBlock.hostData;

    //memset(indices, 0, sizeof(Tanto_R_Index) * mesh.indexCount);
    //memset(positions, 0, sizeof(Tanto_R_Vertex) * mesh.vertexCount);
    
    // make cube
    
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

    const Vec3 colors[6] = {
        {  0.1,  0.5,  1.0 },
        {  0.4,  0.2,  1.0 },
        {  0.0,  1.0,  0.0 },
        {  0.2,  0.8,  0.0 },
        {  1.0,  0.0,  0.0 },
        {  0.5,  1.0,  0.0 },
    };

    const Tanto_R_Attribute uvws[4] = {
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

    for (int i = 0; i < vertCount; i++) 
    {
        pColors[i]  = colors[ i / 4 ];
        //pUvws[i]    = uvws[ i % 4 ];
    }
    for (int i = 0; i < vertCount; i += 4) 
    {
        pUvws[i + 1] = uvws[0];
        pUvws[i + 0] = uvws[1];
        pUvws[i + 2] = uvws[2];
        pUvws[i + 3] = uvws[3];
    }

    //
    // face 0: +Z
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

    pUvws[1] = uvws[0];
    pUvws[0] = uvws[1];
    pUvws[2] = uvws[2];
    pUvws[3] = uvws[3];

    if (CW == 1)
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
    return mesh;
}
