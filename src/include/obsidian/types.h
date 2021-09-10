#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint32_t Obdn_Mask;
typedef uint32_t Obdn_Flags;

typedef enum {
    OBDN_SUCCESS = 1,
    OBDN_ERROR_GENERIC = -1,
    OBDN_ERROR_INSTANCE_EXTENSION_NOT_PRESENT = -2,
    OBDN_ERROR_DEVICE_EXTENSION_NOT_PRESENT = -3,
} Obdn_Result;


#endif /* end of include guard: TYPES_H */
