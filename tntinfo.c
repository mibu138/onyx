#include "tanto/f_file.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file.tnt\n", argv[0]);
        return 2;
    }
    Tanto_F_Primitive fprim;
    int r = tanto_f_ReadPrimitive(argv[1], &fprim);
    if (r != 1)
    {
        fprintf(stderr, "Error reading file.\n");
        return 2;
    }
    tanto_f_PrintPrim(&fprim);
    tanto_f_FreePrimitive(&fprim);
    return 0;
}
