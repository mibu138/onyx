 #ifndef OBDN_TEXT_H
 #define OBDN_TEXT_H

#include "v_image.h"

Obdn_V_Image obdn_t_CreateTextImage(const size_t width, const size_t height, 
        const size_t x, const size_t y,
        const size_t fontSize, const char* text);
 
 #endif /* end of include guard: OBDN_TEXT_H */
