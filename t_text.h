 #ifndef TANTO_TEXT_H
 #define TANTO_TEXT_H

#include "v_image.h"

Tanto_V_Image tanto_CreateTextImage(const size_t width, const size_t height, 
        const size_t x, const size_t y,
        const size_t fontSize, const char* text);
 
 #endif /* end of include guard: TANTO_TEXT_H */
