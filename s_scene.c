#include "s_scene.h"
#include "f_file.h"
#include "coal/m_math.h"
#include "coal/util.h"
#include "tanto/r_geo.h"
#include <string.h>
#define ARCBALL_CAMERA_IMPLEMENTATION
#include "arcball_camera.h"

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
    const Primitive quad = tanto_r_CreateQuad(3, 4, TANTO_R_ATTRIBUTE_UVW_BIT | TANTO_R_ATTRIBUTE_NORMAL_BIT);

    Xform cubeXform = m_Ident_Mat4();
    Xform quadXform = m_Ident_Mat4();

    cubeXform = m_Translate_Mat4((Vec3){0, 0.5, 0}, &cubeXform);

    Mat4 rot = m_BuildRotate(M_PI / 2, &(Vec3){-1, 0, 0});
    quadXform = m_Mult_Mat4(&quadXform, &rot);

    printf(">>Quad Rot\n");
    coal_PrintMat4(&rot);
    printf(">>Quad Xform\n");
    coal_PrintMat4(&quadXform);

    const Light dirLight = {
        .type = TANTO_S_LIGHT_TYPE_DIRECTION,
        .intensity = 1,
        .color     = (Vec3){1, 1, 1},
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

    const Material cubeMat = {
        .color = (Vec3){0.05, 0.18, 0.516},
        .id    = 0
    };

    const Material quadMat = {
        .color = (Vec3){0.75, 0.422, 0.245},
        .id    = 1
    };
    

    Material mats[2] = { 
        cubeMat,
        quadMat
    };

    memcpy(s.prims, prims, sizeof(prims));
    memcpy(s.xforms, xforms, sizeof(xforms));
    memcpy(s.materials, mats, sizeof(mats));

    *scene = s;
}

void tanto_s_CreateSimpleScene3(Scene *scene)
{
    const Primitive cube = tanto_r_CreateCubePrim(false);
    const Primitive quad = tanto_r_CreateQuad(3, 4, TANTO_R_ATTRIBUTE_UVW_BIT | TANTO_R_ATTRIBUTE_NORMAL_BIT);

    Xform cubeXform = m_Ident_Mat4();
    Xform quadXform = m_Ident_Mat4();

    cubeXform = m_Translate_Mat4((Vec3){0, 0.5, 0}, &cubeXform);

    Mat4 rot = m_BuildRotate(M_PI / 2, &(Vec3){-1, 0, 0});
    quadXform = m_Mult_Mat4(&quadXform, &rot);

    printf(">>Quad Rot\n");
    coal_PrintMat4(&rot);
    printf(">>Quad Xform\n");
    coal_PrintMat4(&quadXform);

    const Light dirLight = {
        .type = TANTO_S_LIGHT_TYPE_DIRECTION,
        .intensity = 1,
        .color     = (Vec3){1, 1, 1},
        .structure.directionLight.dir = (Vec3){-0.37904902,  -0.53066863, -0.75809804}
    };

    const Light dirLight2 = {
        .type = TANTO_S_LIGHT_TYPE_DIRECTION,
        .intensity = 1,
        .color     = (Vec3){1, 1, 1},
        .structure.directionLight.dir = (Vec3){0.37904902,  0.53066863, 0.75809804}
    };

    Vec3 pos = (Vec3){0, 0, 2};
    Vec3 tar = (Vec3){0, 0, 0};
    Vec3 up  = (Vec3){0, 1, 0};
    Mat4 cameraXform = m_LookAt(&pos, &tar, &up);

    Scene s = {
        .lightCount = 2,
        .lights = {dirLight, dirLight2},
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

    const Material cubeMat = {
        .color = (Vec3){0.05, 0.18, 0.516},
        .id    = 0
    };

    const Material quadMat = {
        .color = (Vec3){0.75, 0.422, 0.245},
        .id    = 1
    };
    

    Material mats[2] = { 
        cubeMat,
        quadMat
    };

    memcpy(s.prims, prims, sizeof(prims));
    memcpy(s.xforms, xforms, sizeof(xforms));
    memcpy(s.materials, mats, sizeof(mats));

    *scene = s;
}

void tanto_s_CreateEmptyScene(Scene* scene)
{
    memset(scene, 0, sizeof(Scene));
    Mat4 m = m_LookAt(&(Vec3){1, 1, 2}, &(Vec3){0, 0, 0}, &(Vec3){0, 1, 0});
    scene->camera.xform = m;
}

Tanto_S_PrimId tanto_s_LoadPrim(Scene* scene, const char* filePath, const Mat4* xform)
{
    Tanto_F_Primitive fprim;
    int r = tanto_f_ReadPrimitive(filePath, &fprim);
    assert(r);
    Tanto_R_Primitive prim = tanto_f_CreateRPrimFromFPrim(&fprim);
    tanto_f_FreePrimitive(&fprim);
    const uint32_t curIndex = scene->primCount++;
    assert(curIndex < TANTO_S_MAX_PRIMS);
    scene->prims[curIndex] = prim;
    const Mat4 m = xform ? *xform : m_Ident_Mat4();
    scene->xforms[curIndex] = m;
    return curIndex;
}

Tanto_S_LightId tanto_s_CreateDirectionLight(Scene* scene, const Vec3 direction)
{
    Light light = {
        .color = {1, 1, 1},
        .intensity = 1,
        .type = TANTO_S_LIGHT_TYPE_DIRECTION,
        .structure.directionLight.dir = m_Normalize_Vec3(&direction)
    };

    const uint32_t curIndex = scene->lightCount++;
    assert(curIndex < TANTO_S_MAX_LIGHTS);
    scene->lights[curIndex] = light;
    return curIndex;
}

#define HOME_POS    {0.0, 0.0, 1.0}
#define HOME_TARGET {0.0, 0.0, 0.0}
#define HOME_UP     {0.0, 1.0, 0.0}
#define ZOOM_RATE 0.01
#define PAN_RATE 1
#define TUMBLE_RATE 2

void tanto_s_UpdateCamera(Scene* scene, float dt, int16_t mx, int16_t my, bool panning, bool tumbling, bool zooming, bool home)
{
    static Vec3 pos    = HOME_POS;
    static Vec3 target = HOME_TARGET;
    static Vec3 up     = HOME_UP;
    static int xPrev = 0, yPrev = 0;
    static int zoom_ticks = 0;
    if (zooming)
        zoom_ticks = mx - xPrev;
    else
        zoom_ticks = 0;
    if (home)
    {
        pos =    (Vec3)HOME_POS;
        target = (Vec3)HOME_TARGET;
        up =     (Vec3)HOME_UP;
    }
    //pos = m_RotateY_Vec3(dt, &pos);
    arcball_camera_update(pos.x, target.x, up.x, NULL, dt, 
            ZOOM_RATE, PAN_RATE, TUMBLE_RATE, TANTO_WINDOW_WIDTH, TANTO_WINDOW_HEIGHT, xPrev, mx, yPrev, my, 
            panning, tumbling, zoom_ticks, 0);
    Mat4 m = m_LookAt(&pos, &target, &up);
    scene->camera.xform = m;
    xPrev = mx;
    yPrev = my;
    scene->dirt |= TANTO_S_CAMERA_BIT;
}

void tanto_s_UpdateLight(Scene* scene, uint32_t id, float intensity)
{
    assert(id < scene->lightCount);
    scene->lights[id].intensity = intensity;
    scene->dirt |= TANTO_S_LIGHTS_BIT;
}
