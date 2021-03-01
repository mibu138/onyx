#ifndef OBDN_F_FILE_H
#define OBDN_F_FILE_H

#include <stdint.h>
#include "r_geo.h"
#include "f_prim.h"

// Must be freed
#ifdef __cplusplus
Obdn_F_Primitive obdn_f_CreatePrimitive(const uint32_t vertexCount, const uint32_t indexCount, 
        const uint32_t attrCount, 
        const Obdn_R_AttributeSize attrSizes[], 
        const char attrNames[][OBDN_R_ATTR_NAME_LEN]);
#else
Obdn_F_Primitive obdn_f_CreatePrimitive(const uint32_t vertexCount, const uint32_t indexCount, 
        const uint32_t attrCount, 
        const Obdn_R_AttributeSize attrSizes[attrCount], 
        const char attrNames[attrCount][OBDN_R_ATTR_NAME_LEN]);
#endif
Obdn_F_Primitive obdn_f_CreateFPrimFromRPrim(const Obdn_R_Primitive* rprim);
Obdn_R_Primitive obdn_f_CreateRPrimFromFPrim(const Obdn_F_Primitive* fprim);
int               obdn_f_WritePrimitive(const char* filename, const Obdn_F_Primitive* fprim);
// 1 is success
int               obdn_f_ReadPrimitive(const char* filename, Obdn_F_Primitive* fprim);
void              obdn_f_FreePrimitive(Obdn_F_Primitive* fprim);
void              obdn_f_PrintPrim(const Obdn_F_Primitive* prim);
Obdn_R_Primitive obdn_f_LoadRPrim(const char* filename, const bool transferToDevice);

#endif /* end of include guard: OBDN_F_FILE_H */
