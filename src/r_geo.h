#ifndef OBDN_R_GEO_H
#define OBDN_R_GEO_H

#include "v_memory.h"
#include <stdint.h>
#include <coal/coal.h>

#define OBDN_R_MAX_VERT_ATTRIBUTES 8
#define OBDN_R_ATTR_NAME_LEN 4

typedef uint32_t   Obdn_R_Index;
typedef uint8_t    Obdn_R_AttributeSize;

// vertexRegion.offset is the byte offset info the buffer where the vertex data
// is kept. attrOffsets store the byte offset relative to the vertexRegion offset
// where the individual attribute data is kept
// attrSizes stores how many bytes each individual attribute element takes up
// attrCount is how many different attribute types the primitive holds
// vertexRegion.size is the total size of the vertex attribute data
typedef struct Obdn_R_Primitive {
    uint32_t              vertexCount;
    Obdn_V_BufferRegion  vertexRegion;
    uint32_t              indexCount;
    Obdn_V_BufferRegion  indexRegion;
    uint32_t              attrCount;
    char                  attrNames[OBDN_R_MAX_VERT_ATTRIBUTES][OBDN_R_ATTR_NAME_LEN];
    Obdn_R_AttributeSize attrSizes[OBDN_R_MAX_VERT_ATTRIBUTES]; // individual element sizes
    VkDeviceSize          attrOffsets[OBDN_R_MAX_VERT_ATTRIBUTES];
} Obdn_R_Primitive;

typedef struct {
    uint32_t bindingCount;
    uint32_t attributeCount;
    VkVertexInputBindingDescription   bindingDescriptions[OBDN_R_MAX_VERT_ATTRIBUTES];
    VkVertexInputAttributeDescription attributeDescriptions[OBDN_R_MAX_VERT_ATTRIBUTES];
} Obdn_R_VertexDescription;

typedef enum {
    OBDN_R_ATTRIBUTE_NORMAL_BIT = 0x00000001,
    OBDN_R_ATTRIBUTE_UVW_BIT    = 0x00000002
} Obdn_R_AttributeBits;

// pos and color. clockwise for now.
Obdn_R_Primitive  obdn_r_CreateTriangle(void);
Obdn_R_Primitive  obdn_r_CreateCubePrim(const bool isClockWise);
Obdn_R_Primitive  obdn_r_CreateCubePrimUV(const bool isClockWise);
Obdn_R_Primitive  obdn_r_CreatePoints(const uint32_t count);
Obdn_R_Primitive  obdn_r_CreateCurve(const uint32_t vertCount, const uint32_t patchSize, const uint32_t restartOffset);
Obdn_R_Primitive  obdn_r_CreateQuad(const float width, const float height, const Obdn_R_AttributeBits attribBits);
Obdn_R_Primitive  obdn_r_CreateQuadNDC(const float x, const float y, const float width, const float height);
#ifdef __cplusplus // no real reason to be doing this... other than documentation
Obdn_R_Primitive  obdn_r_CreatePrimitive(const uint32_t vertCount, const uint32_t indexCount, 
                                           const uint8_t attrCount, const uint8_t* attrSizes);
Obdn_R_VertexDescription obdn_r_GetVertexDescription(const uint32_t attrCount, const Obdn_R_AttributeSize* attrSizes);
#else
Obdn_R_Primitive  obdn_r_CreatePrimitive(const uint32_t vertCount, const uint32_t indexCount, 
                                           const uint8_t attrCount, const uint8_t attrSizes[attrCount]);
Obdn_R_VertexDescription obdn_r_GetVertexDescription(const uint32_t attrCount, const Obdn_R_AttributeSize attrSizes[attrCount]);
#endif
void*              obdn_r_GetPrimAttribute(const Obdn_R_Primitive* prim, const uint32_t index);
Obdn_R_Index*     obdn_r_GetPrimIndices(const Obdn_R_Primitive* prim);
void obdn_r_BindPrim(const VkCommandBuffer cmdBuf, const Obdn_R_Primitive* prim);
void obdn_r_DrawPrim(const VkCommandBuffer cmdBuf, const Obdn_R_Primitive* prim);
void obdn_r_TransferPrimToDevice(Obdn_R_Primitive* prim);
void obdn_r_FreePrim(Obdn_R_Primitive* prim);
void obdn_r_PrintPrim(const Obdn_R_Primitive* prim);

VkDeviceSize obdn_r_GetAttrOffset(const Obdn_R_Primitive* prim, const char* attrname);
VkDeviceSize obdn_r_GetAttrRange(const Obdn_R_Primitive* prim, const char* attrname);

#endif /* end of include guard: R_GEO_H */
