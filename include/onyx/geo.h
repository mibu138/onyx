#ifndef ONYX_R_GEO_H
#define ONYX_R_GEO_H

#include "memory.h"
#include <stdint.h>

#define ONYX_R_MAX_VERT_ATTRIBUTES 8
#define ONYX_R_ATTR_NAME_LEN 4

typedef uint32_t      Onyx_GeoIndex;
typedef Onyx_GeoIndex Onyx_AttrIndex;
typedef uint8_t       Onyx_GeoAttributeSize;

// vertexRegion.offset is the byte offset info the buffer where the vertex data
// is kept. attrOffsets store the byte offset relative to the vertexRegion
// offset where the individual attribute data is kept attrSizes stores how many
// bytes each individual attribute element takes up attrCount is how many
// different attribute types the primitive holds vertexRegion.size is the total
// size of the vertex attribute data
typedef struct Onyx_Geometry {
    uint32_t          vertexCount;
    uint32_t          indexCount;
    uint32_t          attrCount;
    Onyx_BufferRegion vertexRegion;
    Onyx_BufferRegion indexRegion;
    char attrNames[ONYX_R_MAX_VERT_ATTRIBUTES][ONYX_R_ATTR_NAME_LEN];
    Onyx_GeoAttributeSize
        attrSizes[ONYX_R_MAX_VERT_ATTRIBUTES]; // individual element sizes
    VkDeviceSize attrOffsets[ONYX_R_MAX_VERT_ATTRIBUTES];
} Onyx_Geometry;

typedef struct {
    uint32_t bindingCount;
    uint32_t attributeCount;
    VkVertexInputBindingDescription
        bindingDescriptions[ONYX_R_MAX_VERT_ATTRIBUTES];
    VkVertexInputAttributeDescription
        attributeDescriptions[ONYX_R_MAX_VERT_ATTRIBUTES];
} Onyx_VertexDescription;

// pos and color. clockwise for now.
Onyx_Geometry onyx_CreateTriangle(Onyx_Memory*);
Onyx_Geometry onyx_CreateCube(Onyx_Memory* memory, const bool isClockWise);
Onyx_Geometry onyx_CreateCubeWithTangents(Onyx_Memory* memory, const bool isClockWise);
Onyx_Geometry onyx_CreatePoints(Onyx_Memory*, const uint32_t count);
Onyx_Geometry onyx_CreateCurve(Onyx_Memory*, const uint32_t vertCount,
                               const uint32_t patchSize,
                               const uint32_t restartOffset);
Onyx_Geometry onyx_CreateQuadNDC(Onyx_Memory*, const float x, const float y,
                                 const float width, const float height);
// creates pos, normal, and uv attribute
Onyx_Geometry onyx_CreateQuadNDC_2(Onyx_Memory* memory, const float x,
                                   const float y, const float width,
                                   const float height);

// if the geometry is going to be used for compute or raytracing must pass
// storage buffer usage bit if the geometry is going to be used for acceleration
// structure creation must pass acceleration structure build usage flag
Onyx_Geometry onyx_CreateGeometry(Onyx_Memory*       memory,
                                  VkBufferUsageFlags extraBufferFlags,
                                  const uint32_t     vertCount,
                                  const uint32_t     indexCount,
                                  const uint8_t      attrCount,
                                  const uint8_t      attrSizes[/*attrCount*/]);

// allows passing of attrib names
Onyx_Geometry onyx_CreateGeometry2(Onyx_Memory*       memory,
                                  VkBufferUsageFlags extraBufferFlags,
                                  const uint32_t     vertCount,
                                  const uint32_t     indexCount,
                                  const uint8_t      attrCount,
                                  const uint8_t      attrSizes[/*attrCount*/],
                                  const char*        attrNames[/*attrCount*/]);
Onyx_VertexDescription
      onyx_GetVertexDescription(const uint32_t              attrCount,
                                const Onyx_GeoAttributeSize attrSizes[/*attrCount*/]);
void* onyx_GetGeoAttribute(const Onyx_Geometry* prim, const uint32_t index);
void* onyx_GetGeoAttribute2(const Onyx_Geometry* prim, const char* name);
Onyx_GeoIndex* onyx_GetGeoIndices(const Onyx_Geometry* prim);
void onyx_BindGeo(const VkCommandBuffer cmdBuf, const Onyx_Geometry* prim);
void onyx_DrawGeo(const VkCommandBuffer cmdBuf, const Onyx_Geometry* prim);
void onyx_TransferGeoToDevice(Onyx_Memory* memory, Onyx_Geometry* prim);
void onyx_FreeGeo(Onyx_Geometry* prim);
void onyx_PrintGeo(const Onyx_Geometry* prim);

VkDeviceSize onyx_GetAttrOffset(const Onyx_Geometry* prim,
                                const char*          attrname);
VkDeviceSize onyx_GetAttrOffset2(const Onyx_Geometry* prim, u32 index);
VkDeviceSize onyx_GetAttrRange(const Onyx_Geometry* prim, const char* attrname);

int onyx_GetAttrIndex(const Onyx_Geometry* geo, const char* attrname);

#endif /* end of include guard: R_GEO_H */
