#include <hell/hell.h>
#include <obsidian.h>

Obdn_Instance*  instance;
Obdn_Memory*    memory;
Obdn_Scene*     scene;

int main(int argc, char *argv[])
{
    instance = obdn_AllocInstance();
    memory   = obdn_AllocMemory();
    scene     = obdn_AllocScene();
    obdn_CreateInstance(true, false, 0, NULL, instance);
    obdn_CreateMemory(instance, 100, 100, 100, 0, 0, memory);
    obdn_CreateScene(memory, 0.01, 100, scene);

    Obdn_Geometry cube = obdn_CreateCube(memory, false);
    obdn_PrintGeo(&cube);
    assert(cube.indexCount  == 36);
    assert(cube.vertexCount == 24);
    Obdn_Geometry tri  = obdn_CreateTriangle(memory);
    obdn_PrintGeo(&tri);
    assert(tri.vertexCount == 3);
    assert(tri.indexCount  == 3);


    return 0;
}
