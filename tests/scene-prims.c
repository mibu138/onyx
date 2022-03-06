#include <hell/hell.h>
#include <hell/len.h>
#include <onyx/onyx.h>

Hell_Grimoire* grim;
Hell_EventQueue* equeue;

Onyx_Instance*  instance;
Onyx_Memory*    memory;
Onyx_Scene*     scene;

int main(int argc, char *argv[])
{
    grim = hell_AllocGrimoire();
    equeue = hell_AllocEventQueue();

    hell_CreateEventQueue(equeue);
    hell_CreateGrimoire(equeue, grim);

    instance = onyx_AllocInstance();
    memory   = onyx_AllocMemory();
    scene     = onyx_AllocScene();
    #if UNIX
    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    };
    #elif WIN32
    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    #endif
    Onyx_InstanceParms ip = {
        .enabledInstanceExentensionCount = LEN(instanceExtensions),
        .ppEnabledInstanceExtensionNames = instanceExtensions,
    };
    onyx_CreateInstance(&ip, instance);
    onyx_CreateMemory(instance, 100, 100, 100, 0, 0, memory);
    onyx_CreateScene(grim, memory, 1, 1, 0.01, 100, scene);

    Onyx_Geometry cube = onyx_CreateCube(memory, false);
    onyx_PrintGeo(&cube);
    assert(cube.indexCount  == 36);
    assert(cube.vertexCount == 24);
    Onyx_Geometry tri  = onyx_CreateTriangle(memory);
    onyx_PrintGeo(&tri);
    assert(tri.vertexCount == 3);
    assert(tri.indexCount  == 3);


    return 0;
}
