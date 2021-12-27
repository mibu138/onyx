#ifndef OBSIDIAN_FILE_GEO_H
#define OBSIDIAN_FILE_GEO_H

#include <stdint.h>

typedef struct {
    uint32_t  attrCount;
    uint32_t  vertexCount;
    uint32_t  indexCount;
    uint32_t  padding;
    uint8_t*  attrSizes;
    char**    attrNames;
    void**    attributes;
    uint32_t* indices;
} Obdn_FileGeo;


#endif /* end of include guard: OBSIDIAN_FILE_GEO_H */

