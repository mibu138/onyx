#ifndef R_GEO_H
#define R_GEO_H

#include <stdint.h>
#include "v_memory.h"
#include "m_math.h"

typedef Vec3 Attribute;
typedef uint32_t Index;

typedef struct {
    uint32_t  vertexCount;
    V_block*  vertexBlock;
    VkDeviceSize posOffset;
    VkDeviceSize colOffset;
    VkDeviceSize norOffset;
    VkDeviceSize uvwOffset;
    uint32_t  indexCount;
    V_block*  indexBlock;
} Mesh;

typedef struct {
    uint32_t   vertexCount;
    Attribute* posData;
    Attribute* colData;
    Attribute* norData;
    Attribute* uvwData;
    Index*     indexData;
} PreMesh;

typedef struct {
    Vec3 pos;
    Vec3 color;
    Vec3 normal;
    Vec3 uvw;
} Vertex;

Mesh r_PreMeshToMesh(const PreMesh);

Mesh r_CreateMesh(uint32_t vertexCount, uint32_t indexCount);

Mesh r_CreateCube(void);

#endif /* end of include guard: R_GEO_H */
