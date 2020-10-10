#ifndef TANTO_R_GEO_H
#define TANTO_R_GEO_H

#include <stdint.h>
#include "v_memory.h"
#include "m_math.h"

typedef Vec3 Tanto_R_Attribute;
typedef uint32_t Tanto_R_Index;

typedef struct {
    uint32_t  vertexCount;
    Tanto_V_BlockHostBuffer*  vertexBlock;
    VkDeviceSize posOffset;
    VkDeviceSize colOffset;
    VkDeviceSize norOffset;
    VkDeviceSize uvwOffset;
    uint32_t  indexCount;
    Tanto_V_BlockHostBuffer*  indexBlock;
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

Tanto_R_Mesh tanto_r_PreMeshToMesh(const Tanto_R_PreMesh);

Tanto_R_Mesh tanto_r_CreateMesh(uint32_t vertexCount, uint32_t indexCount);

Tanto_R_Mesh tanto_r_CreateCube(void);

#endif /* end of include guard: R_GEO_H */
