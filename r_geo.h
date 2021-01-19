#ifndef TANTO_R_GEO_H
#define TANTO_R_GEO_H

#include <stdint.h>
#include "v_memory.h"
#include "t_def.h"
#include <coal/coal.h>

#define TANTO_R_MAX_VERT_ATTRIBUTES 8

typedef Vec3       Tanto_R_Attribute;
typedef uint32_t   Tanto_R_Index;
typedef Tanto_Mask Tanto_R_AttributeMask;

typedef enum {
    TANTO_R_ATTRIBUTE_NORMAL_BIT = 0x00000001,
    TANTO_R_ATTRIBUTE_UVW_BIT    = 0x00000002
} Tanto_R_AttributeBits;

typedef struct {
    uint32_t     vertexCount;
    Tanto_V_BufferRegion vertexRegion;
    uint32_t     indexCount;
    Tanto_V_BufferRegion indexRegion;
    uint32_t     attrCount;
    VkDeviceSize attrOffsets[TANTO_R_MAX_VERT_ATTRIBUTES];
} Tanto_R_Primitive;

typedef struct {
    uint32_t bindingCount;
    uint32_t attributeCount;
    VkVertexInputBindingDescription   bindingDescriptions[TANTO_R_MAX_VERT_ATTRIBUTES];
    VkVertexInputAttributeDescription attributeDescriptions[TANTO_R_MAX_VERT_ATTRIBUTES];
} Tanto_R_VertexDescription;

// pos and color. clockwise for now.
Tanto_R_Primitive  tanto_r_CreateTriangle(void);
Tanto_R_Primitive  tanto_r_CreateCubePrim(const bool isClockWise);
Tanto_R_Primitive  tanto_r_CreatePoints(const uint32_t count);
Tanto_R_Primitive  tanto_r_CreateCurve(const uint32_t vertCount, const uint32_t patchSize, const uint32_t restartOffset);
Tanto_R_Primitive  tanto_r_CreateQuad(const float width, const float height, const Tanto_R_AttributeBits attribBits);
Tanto_R_Primitive  tanto_r_CreateQuadNDC(const float x, const float y, const float width, const float height, Tanto_R_VertexDescription* desc);
Tanto_R_Primitive  tanto_r_CreatePrimitive(const uint32_t vertCount, const uint32_t indexCount, const uint8_t attrCount);
Tanto_R_Attribute* tanto_r_GetPrimAttribute(const Tanto_R_Primitive* prim, uint32_t index);
Tanto_R_Index*     tanto_r_GetPrimIndices(const Tanto_R_Primitive* prim);
Tanto_R_VertexDescription tanto_r_GetVertexDescription3D_4Vec3(void);
Tanto_R_VertexDescription tanto_r_GetVertexDescription3D_3Vec3(void);
Tanto_R_VertexDescription tanto_r_GetVertexDescription3D_2Vec3(void);
void tanto_r_BindPrim(const VkCommandBuffer cmdBuf, const Tanto_R_Primitive* prim);
void tanto_r_DrawPrim(const VkCommandBuffer cmdBuf, const Tanto_R_Primitive* prim);
void tanto_r_TransferPrimToDevice(Tanto_R_Primitive* prim);
void tanto_r_FreePrim(Tanto_R_Primitive* prim);

#endif /* end of include guard: R_GEO_H */
