#ifndef OBSIDIAN_R_API_H
#define OBSIDIAN_R_API_H

#include "s_scene.h"

typedef struct {
    VkSemaphore       (*renderUi)(VkSemaphore waitSemephore);
    bool              (*presentFrame)(VkSemaphore waitSemephore);
} Obdn_R_Import;

typedef struct {
    void (*init)(const Obdn_S_Scene* scene);
    void (*render)(VkSemaphore waitSemephore);
} Obdn_R_Export;

typedef Obdn_R_Export (*Obdn_R_Handshake)(Obdn_R_Import import);

#endif /* end of include guard: OBSIDIAN_R_API_H */
