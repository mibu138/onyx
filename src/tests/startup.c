#include <hell/hell.h>
#include <hell/len.h>
#include <obsidian/obsidian.h>

static Obdn_Instance*  oInstance;
static Obdn_Memory*    oMemory;

int main(int argc, char *argv[])
{
    oInstance = obdn_AllocInstance();
    oMemory   = obdn_AllocMemory();
    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    };
    Obdn_InstanceParms ip = {
        .enabledInstanceExentensionCount = LEN(instanceExtensions),
        .ppEnabledInstanceExtensionNames = instanceExtensions,
    };
    obdn_CreateInstance(&ip, oInstance);
    obdn_CreateMemory(oInstance, 100, 100, 100, 0, 0, oMemory);
    return 0;
}
