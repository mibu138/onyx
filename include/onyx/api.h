#ifndef ONYX_R_API_H
#define ONYX_R_API_H

#include "scene.h"

typedef struct {
    VkSemaphore    (*renderUi)(VkSemaphore waitSemephore);
    bool           (*presentFrame)(VkSemaphore waitSemephore);
    //VkExtent2D  (*getWindowDimensions)(void); now window dimensions are parts of the scene
} Onyx_R_Import;

typedef struct {
    void        (*init)(const Onyx_S_Scene* scene, VkImageLayout finalImageLayout, bool openglStyle);
    uint8_t     (*getMaxFramesInFlight)(void);
    VkSemaphore (*render)(uint32_t frameIndex, VkSemaphore waitSemephore);
    void        (*cleanUp)(void);
} Onyx_R_Export;

typedef Onyx_R_Export (*Onyx_R_Handshake)(Onyx_R_Import import);

#endif /* end of include guard: ONYX_R_API_H */
