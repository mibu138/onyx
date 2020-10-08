#include "t_utils.h"
#include <stdio.h>
#include <stdint.h>

void printVec2(const Vec2* vec)
{
    printf("x: %f, y: %f\n", vec->x, vec->y);
}

void printVec3(const Vec3* vec)
{
    printf("x: %f, y: %f z: %f\n", vec->x[0], vec->x[1], vec->x[2]);
}


void printMat4(const Mat4 * m)
{
    for (int i = 0; i < 4; i++) 
    {
        for (int j = 0; j < 4; j++) 
        {
            printf("%f ", m->x[i][j]);
        }
        printf("\n");
    }
}

//void bitprintRowLen(const void *const thing, const size_t bitcount, const size_t rowlength)
//{
//    int mask;
//    for (int i = bitcount - 1; i >= 0; i--) {
//        mask = 1 << i;   
//        if (mask & *(uint64_t*)thing)
//            putchar('1');
//        else
//            putchar('0');
//    }
//    putchar('\n');
//}

void bytePrint(const void* const thing, const size_t byteCount)
{
    int mask;
    const uint8_t* base = (uint8_t*)thing;
    for (int i = byteCount - 1; i >= 0; i--) 
    {
        for (int j = 8 - 1; j >= 0; j--) 
        {
            mask = 1 << j;
            if (mask & *(base + i))
                putchar('1');
            else
                putchar('0');
        }
    }
    putchar('\n');
}

void bitprint(const void* const thing, const size_t bitcount)
{
    int mask;
    for (int i = bitcount - 1; i >= 0; i--) {
        mask = 1 << i;   
        if (mask & *(int*)thing)
            putchar('1');
        else
            putchar('0');
    }
    putchar('\n');
}
