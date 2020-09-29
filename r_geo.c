#include "r_geo.h"

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
    mesh.posOffset  = 0 * mesh.vertexCount * sizeof(Attribute);
    mesh.colOffset  = 1 * mesh.vertexCount * sizeof(Attribute);
    mesh.normOffset = 2 * mesh.vertexCount * sizeof(Attribute);
    return mesh;
}

Mesh r_CreateCube(void)
{
    const uint32_t vertCount  = 24;
    const uint32_t indexCount = 36;
    Mesh mesh = r_CreateMesh(vertCount, indexCount);
    Attribute* pPositions = (Attribute*)(mesh.vertexBlock->address + mesh.posOffset);
    Attribute* pColors    = (Attribute*)(mesh.vertexBlock->address + mesh.colOffset);
    Attribute* pNormals   = (Attribute*)(mesh.vertexBlock->address + mesh.normOffset);
    Index*  indices      = (Index*)mesh.indexBlock->address;

    //memset(indices, 0, sizeof(Index) * mesh.indexCount);
    //memset(positions, 0, sizeof(Vertex) * mesh.vertexCount);
    
    // make cube
    
    const Vec3 points[8] = {
        { -0.5, -0.5,  0.5 },
        { -0.5,  0.5,  0.5 },
        {  0.5,  0.5,  0.5 },
        {  0.5, -0.5,  0.5 },
        { -0.5, -0.5, -0.5 },
        { -0.5,  0.5, -0.5 },
        {  0.5,  0.5, -0.5 },
        {  0.5, -0.5, -0.5 },
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
