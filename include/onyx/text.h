 #ifndef ONYX_TEXT_H
 #define ONYX_TEXT_H

#include "image.h"

void onyx_t_UpdateTextImage(Onyx_Memory* memory, const size_t x, const size_t y, const char* text, Onyx_Image* image);
Onyx_Image onyx_t_CreateTextImage(Onyx_Memory* memory, const size_t width, const size_t height, 
        const size_t x, const size_t y,
        const size_t fontSize, const char* text);
 
 #endif /* end of include guard: ONYX_TEXT_H */
