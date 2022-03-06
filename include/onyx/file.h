#ifndef ONYX_F_FILE_H
#define ONYX_F_FILE_H

#include <stdint.h>
#include "geo.h"
#include "filegeo.h"

// Must be freed
Onyx_FileGeo onyx_CreateFileGeo(const uint32_t vertexCount, const uint32_t indexCount, 
        const uint32_t attrCount, 
        const Onyx_GeoAttributeSize attrSizes[/*attrCount*/], 
        const char attrNames[/*attrCount*/][ONYX_R_ATTR_NAME_LEN]);
Onyx_FileGeo onyx_CreateFileGeoFromGeo(Onyx_Memory* memory, const Onyx_Geometry* rprim);
int               onyx_WriteFileGeo(const char* filename, const Onyx_FileGeo* fprim);
// 1 is success
int               onyx_ReadFileGeo(const char* filename, Onyx_FileGeo* fprim);
void              onyx_FreeFileGeo(Onyx_FileGeo* fprim);
void              onyx_PrintFileGeo(const Onyx_FileGeo* prim);
Onyx_Geometry onyx_CreateGeoFromFileGeo(Onyx_Memory* memory, VkBufferUsageFlags extraBufferUsageFlags, const Onyx_FileGeo *fprim);
Onyx_Geometry onyx_LoadGeo(Onyx_Memory* memory, VkBufferUsageFlags extraBufferUsageFlags, const char* filename,
                 const bool transferToDevice);

#endif /* end of include guard: ONYX_F_FILE_H */
