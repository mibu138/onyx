 #ifndef OBDN_TEXT_H
 #define OBDN_TEXT_H

#include "v_image.h"

void obdn_t_UpdateTextImage(Obdn_Memory* memory, const size_t x, const size_t y, const char* text, Obdn_V_Image* image);
Obdn_V_Image obdn_t_CreateTextImage(Obdn_Memory* memory, const size_t width, const size_t height, 
        const size_t x, const size_t y,
        const size_t fontSize, const char* text);
 
 #endif /* end of include guard: OBDN_TEXT_H */
