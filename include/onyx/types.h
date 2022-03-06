#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint32_t Onyx_Mask;
typedef uint32_t Onyx_Flags;

typedef enum {
    ONYX_SUCCESS = 1,
    ONYX_ERROR_GENERIC = -1,
    ONYX_ERROR_INSTANCE_EXTENSION_NOT_PRESENT = -2,
    ONYX_ERROR_DEVICE_EXTENSION_NOT_PRESENT = -3,
} Onyx_Result;


#endif /* end of include guard: TYPES_H */
