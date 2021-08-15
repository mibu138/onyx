#ifndef OBDN_R_INIT_H
#define OBDN_R_INIT_H

#include "memory.h"
#include "command.h"
#include "scene.h"

#define OBDN_VERT_POS_FORMAT VK_FORMAT_R32G32B32_SFLOAT
#define OBDN_VERT_INDEX_TYPE VK_INDEX_TYPE_UINT32

typedef Obdn_V_Image Obdn_R_Frame;

VkFormat obdn_r_GetOffscreenColorFormat(void);
VkFormat obdn_r_GetDepthFormat(void);

#endif /* end of include guard: R_INIT_H */
