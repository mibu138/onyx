#ifndef OBDN_F_FILE_H
#define OBDN_F_FILE_H

#include <stdint.h>
#include "geo.h"
#include "filegeo.h"

// Must be freed
#ifdef __cplusplus
Obdn_F_Primitive obdn_CreateFileGeo(const uint32_t vertexCount, const uint32_t indexCount, 
        const uint32_t attrCount, 
        const Obdn_R_AttributeSize attrSizes[], 
        const char attrNames[][OBDN_R_ATTR_NAME_LEN]);
#else
Obdn_FileGeo obdn_CreateFileGeo(const uint32_t vertexCount, const uint32_t indexCount, 
        const uint32_t attrCount, 
        const Obdn_GeoAttributeSize attrSizes[attrCount], 
        const char attrNames[attrCount][OBDN_R_ATTR_NAME_LEN]);
#endif
Obdn_FileGeo obdn_CreateFileGeoFromGeo(Obdn_Memory* memory, const Obdn_Geometry* rprim);
int               obdn_WriteFileGeo(const char* filename, const Obdn_FileGeo* fprim);
// 1 is success
int               obdn_ReadFileGeo(const char* filename, Obdn_FileGeo* fprim);
void              obdn_FreeFileGeo(Obdn_FileGeo* fprim);
void              obdn_PrintFileGeo(const Obdn_FileGeo* prim);
Obdn_Geometry obdn_CreateGeoFromFileGeo(Obdn_Memory* memory, const Obdn_FileGeo *fprim);
Obdn_Geometry obdn_LoadGeo(Obdn_Memory* memory, const char* filename, const bool transferToDevice);

#endif /* end of include guard: OBDN_F_FILE_H */
