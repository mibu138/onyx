#ifndef OBDN_R_GEO_H
#define OBDN_R_GEO_H

#include "memory.h"
#include <stdint.h>

#define OBDN_R_MAX_VERT_ATTRIBUTES 8
#define OBDN_R_ATTR_NAME_LEN 4

typedef uint32_t      Obdn_GeoIndex;
typedef Obdn_GeoIndex Obdn_AttrIndex;
typedef uint8_t       Obdn_GeoAttributeSize;

// vertexRegion.offset is the byte offset info the buffer where the vertex data
// is kept. attrOffsets store the byte offset relative to the vertexRegion
// offset where the individual attribute data is kept attrSizes stores how many
// bytes each individual attribute element takes up attrCount is how many
// different attribute types the primitive holds vertexRegion.size is the total
// size of the vertex attribute data
typedef struct Obdn_Geometry {
    uint32_t          vertexCount;
    uint32_t          indexCount;
    uint32_t          attrCount;
    Obdn_BufferRegion vertexRegion;
    Obdn_BufferRegion indexRegion;
    char attrNames[OBDN_R_MAX_VERT_ATTRIBUTES][OBDN_R_ATTR_NAME_LEN];
    Obdn_GeoAttributeSize
        attrSizes[OBDN_R_MAX_VERT_ATTRIBUTES]; // individual element sizes
    VkDeviceSize attrOffsets[OBDN_R_MAX_VERT_ATTRIBUTES];
} Obdn_Geometry;

typedef struct {
    uint32_t bindingCount;
    uint32_t attributeCount;
    VkVertexInputBindingDescription
        bindingDescriptions[OBDN_R_MAX_VERT_ATTRIBUTES];
    VkVertexInputAttributeDescription
        attributeDescriptions[OBDN_R_MAX_VERT_ATTRIBUTES];
} Obdn_VertexDescription;

// pos and color. clockwise for now.
Obdn_Geometry obdn_CreateTriangle(Obdn_Memory*);
Obdn_Geometry obdn_CreateCube(Obdn_Memory* memory, const bool isClockWise);
Obdn_Geometry obdn_CreateCubeWithTangents(Obdn_Memory* memory, const bool isClockWise);
Obdn_Geometry obdn_CreatePoints(Obdn_Memory*, const uint32_t count);
Obdn_Geometry obdn_CreateCurve(Obdn_Memory*, const uint32_t vertCount,
                               const uint32_t patchSize,
                               const uint32_t restartOffset);
Obdn_Geometry obdn_CreateQuadNDC(Obdn_Memory*, const float x, const float y,
                                 const float width, const float height);
// creates pos, normal, and uv attribute
Obdn_Geometry obdn_CreateQuadNDC_2(Obdn_Memory* memory, const float x,
                                   const float y, const float width,
                                   const float height);

// if the geometry is going to be used for compute or raytracing must pass
// storage buffer usage bit if the geometry is going to be used for acceleration
// structure creation must pass acceleration structure build usage flag
Obdn_Geometry obdn_CreateGeometry(Obdn_Memory*       memory,
                                  VkBufferUsageFlags extraBufferFlags,
                                  const uint32_t     vertCount,
                                  const uint32_t     indexCount,
                                  const uint8_t      attrCount,
                                  const uint8_t      attrSizes[/*attrCount*/]);
Obdn_VertexDescription
      obdn_GetVertexDescription(const uint32_t              attrCount,
                                const Obdn_GeoAttributeSize attrSizes[/*attrCount*/]);
void* obdn_GetGeoAttribute(const Obdn_Geometry* prim, const uint32_t index);
void* obdn_GetGeoAttribute2(const Obdn_Geometry* prim, const char* name);
Obdn_GeoIndex* obdn_GetGeoIndices(const Obdn_Geometry* prim);
void obdn_BindGeo(const VkCommandBuffer cmdBuf, const Obdn_Geometry* prim);
void obdn_DrawGeo(const VkCommandBuffer cmdBuf, const Obdn_Geometry* prim);
void obdn_TransferGeoToDevice(Obdn_Memory* memory, Obdn_Geometry* prim);
void obdn_FreeGeo(Obdn_Geometry* prim);
void obdn_PrintGeo(const Obdn_Geometry* prim);

VkDeviceSize obdn_GetAttrOffset(const Obdn_Geometry* prim,
                                const char*          attrname);
VkDeviceSize obdn_GetAttrOffset2(const Obdn_Geometry* prim, u32 index);
VkDeviceSize obdn_GetAttrRange(const Obdn_Geometry* prim, const char* attrname);

int obdn_GetAttrIndex(const Obdn_Geometry* geo, const char* attrname);

#endif /* end of include guard: R_GEO_H */
