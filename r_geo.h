#ifndef TANTO_R_GEO_H
#define TANTO_R_GEO_H

#include <stdint.h>
#include "v_memory.h"
#include "m_math.h"

#define TANTO_R_MAX_VERT_ATTRIBUTES 8

typedef Vec3 Tanto_R_Attribute;
typedef uint32_t Tanto_R_Index;

typedef struct {
    uint32_t  vertexCount;
    Tanto_V_BufferRegion vertexBlock;
    VkDeviceSize posOffset;
    VkDeviceSize colOffset;
    VkDeviceSize norOffset;
    VkDeviceSize uvwOffset;
    uint32_t  indexCount;
    Tanto_V_BufferRegion indexBlock;
} Tanto_R_Mesh;

typedef struct {
    uint32_t   vertexCount;
    Tanto_R_Attribute* posData;
    Tanto_R_Attribute* colData;
    Tanto_R_Attribute* norData;
    Tanto_R_Attribute* uvwData;
    Tanto_R_Index*     indexData;
} Tanto_R_PreMesh;

typedef struct {
    Vec3 pos;
    Vec3 color;
    Vec3 normal;
    Vec3 uvw;
} Tanto_R_Vertex;

typedef struct {
    uint32_t     vertexCount;
    Tanto_V_BufferRegion vertexRegion;
    uint32_t     indexCount;
    Tanto_V_BufferRegion indexRegion;
    uint8_t      attrCount;
    VkDeviceSize attrOffsets[TANTO_R_MAX_VERT_ATTRIBUTES];
} Tanto_R_Primitive;

typedef struct {
    uint32_t bindingCount;
    uint32_t attributeCount;
    const VkVertexInputBindingDescription   bindingDescriptions[TANTO_R_MAX_VERT_ATTRIBUTES];
    const VkVertexInputAttributeDescription attributeDescriptions[TANTO_R_MAX_VERT_ATTRIBUTES];
} Tanto_R_VertexDescription;

Tanto_R_Mesh tanto_r_PreMeshToMesh(const Tanto_R_PreMesh);

Tanto_R_Mesh tanto_r_CreateMesh(uint32_t vertexCount, uint32_t indexCount);

Tanto_R_Mesh tanto_r_CreateCube(void);

// pos and color. clockwise for now.
Tanto_R_Primitive tanto_r_CreateTriangle(void);

Tanto_R_Primitive tanto_r_CreatePoints(const uint32_t count);

Tanto_R_Primitive tanto_r_CreateCurve(const uint32_t vertCount, const uint32_t patchSize, const uint32_t restartOffset);

Tanto_R_Primitive tanto_r_CreateEmptySimplePrimitive(const uint32_t vertCount, const uint32_t indexCount);

Tanto_R_VertexDescription tanto_r_GetVertexDescription3D_Default(void);

// Pos and color. 
Tanto_R_VertexDescription tanto_r_GetVertexDescription3D_Simple(void);

void tanto_r_FreeMesh(Tanto_R_Mesh mesh);

#endif /* end of include guard: R_GEO_H */
