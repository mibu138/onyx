#include "s_scene.h"
#include "f_file.h"
#include "coal/m_math.h"
#include "coal/util.h"
#include "r_geo.h"
#include "v_image.h"
#include <string.h>
#define ARCBALL_CAMERA_IMPLEMENTATION
#include "arcball_camera.h"

typedef Obdn_R_Primitive Primitive;
typedef Obdn_S_Xform     Xform;
typedef Obdn_S_Scene     Scene;
typedef Obdn_S_Light     Light;
typedef Obdn_S_Material  Material;
typedef Obdn_S_Camera    Camera;
typedef Obdn_S_Texture   Texture;

// TODO: this function needs to updated
void obdn_s_CreateSimpleScene_NEEDS_UPDATE(Scene *scene)
{
    const Primitive cube = obdn_r_CreateCubePrim(false);
    const Primitive quad = obdn_r_CreateQuad(3, 4, OBDN_R_ATTRIBUTE_UVW_BIT | OBDN_R_ATTRIBUTE_NORMAL_BIT);

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
        .type = OBDN_S_LIGHT_TYPE_DIRECTION,
        .intensity = 1,
        .color     = (Vec3){1, 1, 1},
        .structure.directionLight.dir = (Vec3){-0.37904902,  -0.53066863, -0.75809804}
    };

    const Light dirLight2 = {
        .type = OBDN_S_LIGHT_TYPE_DIRECTION,
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
        .color        = (Vec3){0.05, 0.18, 0.516},
        .roughness    = .5,
        .textureAlbedo    = OBDN_S_NONE,
        .textureRoughness = OBDN_S_NONE,
    };

    const Material quadMat = {
        .color        = (Vec3){0.75, 0.422, 0.245},
        .roughness    = 1,
        .textureAlbedo    = OBDN_S_NONE,
        .textureRoughness = OBDN_S_NONE,
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

#define DEFAULT_MAT_ID 0

void obdn_s_CreateEmptyScene(Scene* scene)
{
    memset(scene, 0, sizeof(Scene));
    Mat4 m = m_LookAt(&(Vec3){1, 1, 2}, &(Vec3){0, 0, 0}, &(Vec3){0, 1, 0});
    scene->camera.xform = m;
    obdn_s_LoadTexture(scene, "data/chungus.jpg", 4); // for debugging, a texId of zero gives you this. if he shows up something went wrong
    obdn_s_CreateMaterial(scene, (Vec3){1, .4, .7}, 1, 0, 0, 0); // default matId
    for (int i = 0; i < OBDN_S_MAX_PRIMS; i++) 
    {
        scene->xforms[i] = m_Ident_Mat4();
    }

    scene->dirt |= OBDN_S_CAMERA_BIT | OBDN_S_TEXTURES_BIT | OBDN_S_MATERIALS_BIT | OBDN_S_XFORMS_BIT;
}

void obdn_s_BindPrimToMaterial(Scene* scene, const Obdn_S_PrimId primId, const Obdn_S_MaterialId matId)
{
    assert(scene->materialCount > matId);
    assert(scene->primCount > primId);
    scene->prims[primId].materialId = matId;

    scene->dirt |= OBDN_S_PRIMS_BIT;
}

Obdn_S_PrimId obdn_s_AddRPrim(Scene* scene, const Obdn_R_Primitive prim, const Mat4* xform)
{
    const uint32_t curIndex = scene->primCount++;
    assert(curIndex < OBDN_S_MAX_PRIMS);
    scene->prims[curIndex].rprim = prim;

    const Mat4 m = xform ? *xform : m_Ident_Mat4();
    scene->xforms[curIndex] = m;

    scene->prims[curIndex].materialId = DEFAULT_MAT_ID;

    scene->dirt |= OBDN_S_XFORMS_BIT | OBDN_S_PRIMS_BIT;

    return curIndex;
}

Obdn_S_PrimId obdn_s_LoadPrim(Scene* scene, const char* filePath, const Mat4* xform)
{
    Obdn_F_Primitive fprim;
    int r = obdn_f_ReadPrimitive(filePath, &fprim);
    assert(r);
    Obdn_R_Primitive prim = obdn_f_CreateRPrimFromFPrim(&fprim);
    obdn_r_TransferPrimToDevice(&prim);
    obdn_f_FreePrimitive(&fprim);
    const uint32_t curIndex = scene->primCount++;
    assert(curIndex < OBDN_S_MAX_PRIMS);
    scene->prims[curIndex].rprim = prim;

    const Mat4 m = xform ? *xform : m_Ident_Mat4();
    scene->xforms[curIndex] = m;

    scene->prims[curIndex].materialId = DEFAULT_MAT_ID;

    scene->dirt |= OBDN_S_XFORMS_BIT | OBDN_S_PRIMS_BIT;

    return curIndex;
}

Obdn_S_TextureId obdn_s_LoadTexture(Obdn_S_Scene* scene, const char* filePath, const uint8_t channelCount)
{
    Texture texture = {0};

    VkFormat format;

    switch (channelCount) 
    {
        case 1: format = VK_FORMAT_R8_UNORM; break;
        case 3: format = VK_FORMAT_R8G8B8A8_UNORM; break;
        case 4: format = VK_FORMAT_R8G8B8A8_UNORM; break;
        default: printf("ChannelCount %d not support.\n", channelCount); return OBDN_S_NONE;
    }

    obdn_v_LoadImage(filePath, channelCount, format,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_IMAGE_ASPECT_COLOR_BIT, 
            1, VK_FILTER_LINEAR, obdn_v_GetQueueFamilyIndex(OBDN_V_QUEUE_GRAPHICS_TYPE), 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, &texture.devImage);

    const Obdn_S_TextureId texId = scene->textureCount++; 
    scene->textures[texId] = texture;
    assert(scene->textureCount < OBDN_S_MAX_TEXTURES);

    scene->dirt |= OBDN_S_TEXTURES_BIT;

    return texId;
}

Obdn_S_MaterialId obdn_s_CreateMaterial(Obdn_S_Scene* scene, Vec3 color, float roughness, 
        Obdn_S_TextureId albedoId, Obdn_S_TextureId roughnessId, Obdn_S_TextureId normalId)
{
    Obdn_S_MaterialId matId = scene->materialCount++;
    assert(matId < OBDN_S_MAX_TEXTURES);

    scene->materials[matId].color     = color;
    scene->materials[matId].roughness = roughness;
    scene->materials[matId].textureAlbedo    = albedoId;
    scene->materials[matId].textureRoughness = roughnessId;
    scene->materials[matId].textureNormal    = normalId;

    scene->dirt |= OBDN_S_MATERIALS_BIT;

    return matId;
}

Obdn_S_LightId obdn_s_CreateDirectionLight(Scene* scene, const Vec3 color, const Vec3 direction)
{
    Light light = {
        .color = color,
        .intensity = 1,
        .type = OBDN_S_LIGHT_TYPE_DIRECTION,
        .structure.directionLight.dir = m_Normalize_Vec3(&direction)
    };

    const uint32_t curIndex = scene->lightCount++;
    assert(curIndex < OBDN_S_MAX_LIGHTS);
    scene->lights[curIndex] = light;

    scene->dirt |= OBDN_S_LIGHTS_BIT;

    return curIndex;
}

Obdn_S_LightId obdn_s_CreatePointLight(Scene* scene, const Vec3 color, const Vec3 position)
{
    Light light = {
        .color = color,
        .intensity = 1,
        .type = OBDN_S_LIGHT_TYPE_POINT,
        .structure.pointLight.pos = position
    };

    const uint32_t curIndex = scene->lightCount++;
    assert(curIndex < OBDN_S_MAX_LIGHTS);
    scene->lights[curIndex] = light;

    scene->dirt |= OBDN_S_LIGHTS_BIT;

    return curIndex;
}

#define HOME_POS    {0.0, 0.0, 1.0}
#define HOME_TARGET {0.0, 0.0, 0.0}
#define HOME_UP     {0.0, 1.0, 0.0}
#define ZOOM_RATE   0.005
#define PAN_RATE    0.1
#define TUMBLE_RATE 2

void obdn_s_UpdateCamera_LookAt(Scene* scene, Vec3 pos, Vec3 target, Vec3 up)
{
    scene->camera.xform = m_LookAt(&pos, &target, &up);
    scene->dirt |= OBDN_S_CAMERA_BIT;
}

void obdn_s_UpdateCamera_ArcBall(Scene* scene, float dt, int16_t mx, int16_t my, bool panning, bool tumbling, bool zooming, bool home)
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
            ZOOM_RATE, PAN_RATE, TUMBLE_RATE, OBDN_WINDOW_WIDTH, OBDN_WINDOW_HEIGHT, xPrev, mx, yPrev, my, 
            panning, tumbling, zoom_ticks, 0);
    Mat4 m = m_LookAt(&pos, &target, &up);
    scene->camera.xform = m;
    xPrev = mx;
    yPrev = my;
    scene->dirt |= OBDN_S_CAMERA_BIT;
}

void obdn_s_UpdateLight(Scene* scene, uint32_t id, float intensity)
{
    assert(id < scene->lightCount);
    scene->lights[id].intensity = intensity;
    scene->dirt |= OBDN_S_LIGHTS_BIT;
}

void obdn_s_UpdatePrimXform(Scene* scene, const Obdn_S_PrimId primId, const Mat4* delta)
{
    assert(primId < scene->primCount);
    scene->xforms[primId] = m_Mult_Mat4(&scene->xforms[primId], delta);
    scene->dirt |= OBDN_S_XFORMS_BIT;
}
