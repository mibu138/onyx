#ifndef OBSIDIAN_R_API_H
#define OBSIDIAN_R_API_H

#include "s_scene.h"

typedef struct {
    VkSemaphore (*renderUi)(VkSemaphore waitSemephore);
    bool        (*presentFrame)(VkSemaphore waitSemephore);
    VkExtent2D  (*getWindowDimensions)(void);
} Obdn_R_Import;

typedef struct {
    void        (*init)(const Obdn_S_Scene* scene);
    uint8_t     (*getMaxFramesInFlight)(void);
    VkSemaphore (*render)(uint32_t frameIndex, VkSemaphore waitSemephore);
    void        (*cleanUp)(void);
} Obdn_R_Export;

typedef Obdn_R_Export (*Obdn_R_Handshake)(Obdn_R_Import import);

#endif /* end of include guard: OBSIDIAN_R_API_H */
