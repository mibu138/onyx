#ifndef ONYX_R_INIT_H
#define ONYX_R_INIT_H

#include "memory.h"
#include "command.h"
#include "scene.h"

#define ONYX_VERT_POS_FORMAT VK_FORMAT_R32G32B32_SFLOAT
#define ONYX_VERT_INDEX_TYPE VK_INDEX_TYPE_UINT32

typedef Onyx_Image Onyx_R_Frame;

VkFormat onyx_r_GetOffscreenColorFormat(void);
VkFormat onyx_r_GetDepthFormat(void);

#endif /* end of include guard: R_INIT_H */
