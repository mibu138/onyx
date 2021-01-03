#include "s_scene.h"
#include "coal/m_math.h"
#include "tanto/r_geo.h"
#include <string.h>

typedef Tanto_R_Primitive Primitive;
typedef Tanto_S_Xform     Xform;
typedef Tanto_S_Scene     Scene;
typedef Tanto_S_Light     Light;
typedef Tanto_S_Matrial   Material;
typedef Tanto_S_Camera    Camera;

void tanto_s_CreateSimpleScene(Scene *scene)
{
    const Primitive cube = tanto_r_CreateCubePrim(false);

    Xform cubeXform = m_Ident_Mat4();
    cubeXform = m_Translate_Mat4((Vec3){0, 0.5, 0}, &cubeXform);
    
    const Light dirLight = {
        .type = TANTO_S_LIGHT_TYPE_DIRECTION,
        .intensity = 1,
        .structure.directionLight.dir = (Vec3){-0.37904902,  -0.53066863, -0.75809804}
    };

    const Material mat = {
        .color = (Vec3){1, 0, 0}
    };

    Vec3 pos = (Vec3){0, 0, 2};
    Vec3 tar = (Vec3){0, 0, 0};
    Vec3 up  = (Vec3){0, 1, 0};
    Mat4 cameraXform = m_LookAt(&pos, &tar, &up);

    const Scene s = {
        .lightCount = 1,
        .lights[0] = dirLight,
        .primCount = 1,
        .prims[0] = cube,
        .xforms[0] = cubeXform,
        .materials[0] = mat,
        .camera = (Camera){
            .xform = cameraXform
        },
        .dirt = -1, // set to all dirty
    };

    *scene = s;
}

void tanto_s_CreateSimpleScene2(Scene *scene)
{
    const Primitive cube = tanto_r_CreateCubePrim(false);
    const Primitive quad = tanto_r_CreateQuad(3, 2, TANTO_R_ATTRIBUTE_UVW_BIT | TANTO_R_ATTRIBUTE_NORMAL_BIT);

    Xform cubeXform = m_Ident_Mat4();
    Xform quadXform = m_Ident_Mat4();

    cubeXform = m_Translate_Mat4((Vec3){0, 0.5, 0}, &cubeXform);
    
    const Light dirLight = {
        .type = TANTO_S_LIGHT_TYPE_DIRECTION,
        .intensity = 1,
        .structure.directionLight.dir = (Vec3){-0.37904902,  -0.53066863, -0.75809804}
    };

    Vec3 pos = (Vec3){0, 0, 2};
    Vec3 tar = (Vec3){0, 0, 0};
    Vec3 up  = (Vec3){0, 1, 0};
    Mat4 cameraXform = m_LookAt(&pos, &tar, &up);

    Scene s = {
        .lightCount = 1,
        .lights[0] = dirLight,
        .primCount = 2,
        .camera = (Camera){
            .xform = cameraXform
        },
        .dirt = -1, // set to all dirty
    };

    Primitive prims[2] = {
        cube,
        quad
    };

    Xform xforms[2] = {
        cubeXform,
        quadXform
    };

    Material mats[2] = { 
        (Vec3){1, 0, 0},
        (Vec3){0, 0, 1}
    };

    memcpy(s.prims, prims, sizeof(prims));
    memcpy(s.xforms, xforms, sizeof(xforms));
    memcpy(s.materials, mats, sizeof(mats));

    *scene = s;
}