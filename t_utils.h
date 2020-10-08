#ifndef T_UTILS_H
#define T_UTILS_H

#include <stdio.h>
#include "m_math.h"

#define printBuffer(pBuffer, size, Type, printFn) { \
    const Type* util_pBuffer = (Type*)pBuffer; \
    for (int i = 0; i < size / sizeof(Type); i++) \
    { \
        printFn(&util_pBuffer[i]); \
    } \
} ;

void printVec2(const Vec2*);
void printVec3(const Vec3*);
void printMat4(const Mat4*);
void bitprintRowLen(const void *const thing, const size_t bitcount, const size_t rowlength);
void bytePrint(const void* const thing, const size_t byteCount);
void bitprint(const void *const thing, const size_t bitcount);

#endif /* end of include guard: UTILS_H */
