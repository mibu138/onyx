#ifndef ONYX_R_GEO_H
#define ONYX_R_GEO_H

#include "memory.h"
#include "attribute.h"
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

typedef enum onyx_GeometryType {
    ONYX_GEOMETRY_TYPE_TRIANGLES = 0,
    ONYX_GEOMETRY_TYPE_POINTS    = 1,
    ONYX_GEOMETRY_TYPE_LINES     = 2,
    ONYX_GEOMETRY_TYPE_MAX
} onyx_GeometryType;

typedef enum onyx_GeometryFlag {
    ONYX_GEOMETRY_FLAG_ARRAY_OF_STRUCTS = 1 << 0,
    ONYX_GEOMETRY_FLAG_UNINDEXED        = 1 << 1,
    ONYX_GEOMETRY_FLAG_INDEX_SIZE_SHORT = 1 << 2,
    ONYX_GEOMETRY_FLAG_NO_TYPES         = 1 << 3,
    ONYX_GEOMETRY_FLAG_MAX
} onyx_GeometryFlag; 

typedef enum OnyxAttributeTypes {
    ONYX_ATTRIBUTE_TYPE_POS,
    ONYX_ATTRIBUTE_TYPE_UV,
    ONYX_ATTRIBUTE_TYPE_NORMAL,
    ONYX_ATTRIBUTE_TYPE_TANGENT,
    ONYX_ATTRIBUTE_TYPE_BITANGENT,
} OnyxAttributeTypes;

typedef uint32_t OnyxFlags;

typedef struct OnyxGeometry {
    uint32_t                type : 2;
    uint32_t                flags : 4;
    uint32_t                attribute_count : 8;
    // if attribute_count is greater than 8 then p_attribute_sizes will point to
    // a separately allocated array where they are stored.
    union {
        uint8_t  arr[8];
        uint8_t* ptr;
    } attribute_sizes;
    // keeping these separate gives us two things. one is that we can
    // use different buffer usage flags for each, which may matter on some
    // devices.
    // more importantly it allows us to more easily put the index buffer and 
    // vertex buffer in separate regions of memory. this simplifies
    // resizing the geometry (imagine resizing the geometry if the indices
    // started right after the vertices).
    Onyx_BufferRegion       vertex_buffer_region;
    // maybe unused
    Onyx_BufferRegion       index_buffer_region;
    // same pattern as sizes. maybe unused.
    union {
        uint8_t  arr[8];
        uint8_t* ptr;
    } attribute_types;
} OnyxGeometry;

typedef struct {
    uint32_t bindingCount;
    uint32_t attributeCount;
    VkVertexInputBindingDescription
        bindingDescriptions[ONYX_R_MAX_VERT_ATTRIBUTES];
    VkVertexInputAttributeDescription
        attributeDescriptions[ONYX_R_MAX_VERT_ATTRIBUTES];
} Onyx_VertexDescription;

typedef struct {
    onyx_Memory* memory;
    onyx_MemoryType memtype;
    onyx_GeometryType type; 
    u32 flags;
    u32 attr_count; 
    u8* attr_sizes;
    u8* attr_types;
    u32 vertex_count;
    u32 index_count;
} OnyxCreateGeometryInfo;

// pos and color. clockwise for now.
OnyxGeometry onyx_create_geometry(const OnyxCreateGeometryInfo* c);
Onyx_Geometry onyx_CreateTriangle(Onyx_Memory*);
Onyx_Geometry onyx_CreateCube(Onyx_Memory* memory, const bool isClockWise);
Onyx_Geometry onyx_CreateCubeWithTangents(Onyx_Memory* memory,
                                          const bool   isClockWise);
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
