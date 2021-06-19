#include <hell/hell.h>
#include <obsidian.h>

static Obdn_Instance*  oInstance;
static Obdn_Memory*    oMemory;

int main(int argc, char *argv[])
{
    oInstance = obdn_AllocInstance();
    oMemory   = obdn_AllocMemory();
    obdn_CreateInstance(true, false, 0, NULL, oInstance);
    obdn_CreateMemory(oInstance, 100, 100, 100, 0, 0, oMemory);
    return 0;
}
