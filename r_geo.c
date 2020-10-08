#include "r_geo.h"
#include <string.h>

const int CW = 1;

Mesh r_PreMeshToMesh(const PreMesh pm)
{
    Mesh m = {};
    size_t nverts = pm.vertexCount;
    m.vertexCount = nverts;
    m.vertexBlock = v_RequestBlock(nverts * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    m.posOffset   = nverts * sizeof(Attribute) * 0;
    m.colOffset   = nverts * sizeof(Attribute) * 1;
    m.norOffset   = nverts * sizeof(Attribute) * 2;
    m.uvwOffset   = nverts * sizeof(Attribute) * 3;
    m.indexBlock  = v_RequestBlock(nverts * sizeof(Index), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    m.indexCount  = nverts;

    memcpy(m.vertexBlock->address + m.posOffset, pm.posData, sizeof(Attribute) * nverts);
    memcpy(m.vertexBlock->address + m.colOffset, pm.colData, sizeof(Attribute) * nverts);
    memcpy(m.vertexBlock->address + m.norOffset, pm.norData, sizeof(Attribute) * nverts);
    memcpy(m.indexBlock->address, pm.indexData, sizeof(Index) * nverts);
    
    return m;
}

Mesh r_CreateMesh(uint32_t vertexCount, uint32_t indexCount)
{
    Mesh mesh;
    mesh.vertexCount = vertexCount;
    mesh.indexCount  = indexCount;
    mesh.vertexBlock = v_RequestBlock(sizeof(Vertex) * mesh.vertexCount, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    mesh.indexBlock  = v_RequestBlock(sizeof(Index) * mesh.indexCount, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    mesh.posOffset = 0 * mesh.vertexCount * sizeof(Attribute);
    mesh.colOffset = 1 * mesh.vertexCount * sizeof(Attribute);
    mesh.norOffset = 2 * mesh.vertexCount * sizeof(Attribute);
    mesh.uvwOffset = 3 * mesh.vertexCount * sizeof(Attribute);
    return mesh;
}

Mesh r_CreateCube(void)
{
    const uint32_t vertCount  = 24;
    const uint32_t indexCount = 36;
    Mesh mesh = r_CreateMesh(vertCount, indexCount);
    Attribute* pPositions = (Attribute*)(mesh.vertexBlock->address + mesh.posOffset);
    Attribute* pColors    = (Attribute*)(mesh.vertexBlock->address + mesh.colOffset);
    Attribute* pNormals   = (Attribute*)(mesh.vertexBlock->address + mesh.norOffset);
    Attribute* pUvws      = (Attribute*)(mesh.vertexBlock->address + mesh.uvwOffset);
    Index*  indices       = (Index*)mesh.indexBlock->address;

    //memset(indices, 0, sizeof(Index) * mesh.indexCount);
    //memset(positions, 0, sizeof(Vertex) * mesh.vertexCount);
    
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
    
    const Vec3 norals[6] = {
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

    const Attribute uvws[4] = {
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
        pUvws[i]    = uvws[ i % 4 ];
    }
    //
    // face 0: +Z
    pNormals[0]  = norals[0];
    pNormals[1]  = norals[0];
    pNormals[2]  = norals[0];
    pNormals[3]  = norals[0];
    // pNormals -Z
    pNormals[4]  = norals[1];
    pNormals[5]  = norals[1];
    pNormals[6]  = norals[1];
    pNormals[7]  = norals[1];
    // pNormals -X
    pNormals[8]  = norals[5];
    pNormals[9]  = norals[5];
    pNormals[10] = norals[5];
    pNormals[11] = norals[5];
    // pNormals +X
    pNormals[12] = norals[4];
    pNormals[13] = norals[4];
    pNormals[14] = norals[4];
    pNormals[15] = norals[4];
    // pNormals +Y
    pNormals[16] = norals[2];
    pNormals[17] = norals[2];
    pNormals[18] = norals[2];
    pNormals[19] = norals[2];
    // pNormals -Y
    pNormals[20] = norals[3];
    pNormals[21] = norals[3];
    pNormals[22] = norals[3];
    pNormals[23] = norals[3];

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
