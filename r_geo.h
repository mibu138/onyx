#ifndef R_GEO_H
#define R_GEO_H

#include <stdint.h>
#include "v_memory.h"
#include "m_math.h"

typedef struct {
    uint32_t  vertexCount;
    V_block*  vertexBlock;
    VkDeviceSize posOffset;
    VkDeviceSize colOffset;
    VkDeviceSize normOffset;
    uint32_t  indexCount;
    V_block*  indexBlock;
} Mesh;

typedef struct {
    Vec3 pos;
    Vec3 color;
    Vec3 normal;
} Vertex;

typedef Vec3 Attribute;
typedef uint32_t Index;

Mesh r_CreateMesh(uint32_t vertexCount, uint32_t indexCount);

Mesh r_CreateCube(void);

#endif /* end of include guard: R_GEO_H */
