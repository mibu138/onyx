#include "obdn/f_file.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s file.tnt\n", argv[0]);
        return 2;
    }
    Obdn_F_Primitive fprim;
    int r = obdn_f_ReadPrimitive(argv[1], &fprim);
    if (r != 1)
    {
        fprintf(stderr, "Error reading file.\n");
        return 2;
    }
    obdn_f_PrintPrim(&fprim);
    obdn_f_FreePrimitive(&fprim);
    return 0;
}
