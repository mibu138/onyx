#include "../include/onyx/types.h"

// msg_buf and spirv_code_buf should be large enough for spirv and message.
// consider heap allocating.
// spirv_code_size will be the size of the spirv code in bytes
// return true on success and false on failure
bool
glsl_to_spirv(const SpirvCompileInfo* info, unsigned char** spirv_code_buf,
              int* spirv_code_size);

