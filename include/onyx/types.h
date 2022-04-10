#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t Onyx_Mask;
typedef uint32_t Onyx_Flags;

typedef enum {
    ONYX_SUCCESS = 1,
    ONYX_ERROR_GENERIC = -1,
    ONYX_ERROR_INSTANCE_EXTENSION_NOT_PRESENT = -2,
    ONYX_ERROR_DEVICE_EXTENSION_NOT_PRESENT = -3,
} Onyx_Result;

typedef enum Onyx_ShaderType {
    ONYX_SHADER_TYPE_VERTEX,
    ONYX_SHADER_TYPE_FRAGMENT,
    ONYX_SHADER_TYPE_COMPUTE,
    ONYX_SHADER_TYPE_GEOMETRY,
    ONYX_SHADER_TYPE_TESS_CONTROL,
    ONYX_SHADER_TYPE_TESS_EVALUATION,
    ONYX_SHADER_TYPE_RAY_GEN,
    ONYX_SHADER_TYPE_ANY_HIT,
    ONYX_SHADER_TYPE_CLOSEST_HIT,
    ONYX_SHADER_TYPE_MISS,
} Onyx_ShaderType;


typedef struct {
    const char* shader_string;
    const char* entry_point;
    const char* name;
    Onyx_ShaderType shader_type;
} SpirvCompileInfo;

#endif /* end of include guard: TYPES_H */
