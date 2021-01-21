#include "s_scene.h"
#include "f_file.h"
#include "coal/m_math.h"
#include "coal/util.h"
#include "tanto/r_geo.h"
#include "tanto/v_image.h"
#include <string.h>
#include <vulkan/vulkan_core.h>
#define ARCBALL_CAMERA_IMPLEMENTATION
#include "arcball_camera.h"

typedef Tanto_R_Primitive Primitive;
typedef Tanto_S_Xform     Xform;
typedef Tanto_S_Scene     Scene;
typedef Tanto_S_Light     Light;
typedef Tanto_S_Material  Material;
typedef Tanto_S_Camera    Camera;
typedef Tanto_S_Texture   Texture;

// TODO: this function needs to updated
void tanto_s_CreateSimpleScene_NEEDS_UPDATE(Scene *scene)
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
        .color        = (Vec3){0.05, 0.18, 0.516},
        .roughness    = .5,
        .textureAlbedo    = TANTO_S_NONE,
        .textureRoughness = TANTO_S_NONE,
    };

    const Material quadMat = {
        .color        = (Vec3){0.75, 0.422, 0.245},
        .roughness    = 1,
        .textureAlbedo    = TANTO_S_NONE,
        .textureRoughness = TANTO_S_NONE,
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
    tanto_s_LoadTexture(scene, "data/chungus.jpg", 4); // for debugging, a tedId of zero gives you this
}

void tanto_s_BindPrimToMaterial(Scene* scene, const Tanto_S_PrimId primId, const Tanto_S_MaterialId matId)
{
    assert(scene->materialCount > matId);
    assert(scene->primCount > primId);
    scene->prims[primId].materialId = matId;
}

Tanto_S_PrimId tanto_s_AddRPrim(Scene* scene, const Tanto_R_Primitive prim, const Mat4* xform)
{
    const uint32_t curIndex = scene->primCount++;
    assert(curIndex < TANTO_S_MAX_PRIMS);
    scene->prims[curIndex].rprim = prim;

    const Mat4 m = xform ? *xform : m_Ident_Mat4();
    scene->xforms[curIndex] = m;

    scene->prims[curIndex].materialId = tanto_s_CreateMaterial(scene, (Vec3){1, .4, .7}, 1, 0, 0);

    scene->dirt |= TANTO_S_XFORMS_BIT;

    return curIndex;
}

Tanto_S_PrimId tanto_s_LoadPrim(Scene* scene, const char* filePath, const Mat4* xform)
{
    Tanto_F_Primitive fprim;
    int r = tanto_f_ReadPrimitive(filePath, &fprim);
    assert(r);
    Tanto_R_Primitive prim = tanto_f_CreateRPrimFromFPrim(&fprim);
    tanto_r_TransferPrimToDevice(&prim);
    tanto_f_FreePrimitive(&fprim);
    const uint32_t curIndex = scene->primCount++;
    assert(curIndex < TANTO_S_MAX_PRIMS);
    scene->prims[curIndex].rprim = prim;

    const Mat4 m = xform ? *xform : m_Ident_Mat4();
    scene->xforms[curIndex] = m;

    scene->prims[curIndex].materialId = tanto_s_CreateMaterial(scene, (Vec3){1, .4, .7}, 1, TANTO_S_NONE, TANTO_S_NONE);

    scene->dirt |= TANTO_S_XFORMS_BIT;

    return curIndex;
}

Tanto_S_TextureId tanto_s_LoadTexture(Tanto_S_Scene* scene, const char* filePath, const uint8_t channelCount)
{
    Texture texture = {0};

    VkFormat format;

    switch (channelCount) 
    {
        case 1: format = VK_FORMAT_R8_UNORM; break;
        case 3: format = VK_FORMAT_R8G8B8A8_UNORM; break;
        case 4: format = VK_FORMAT_R8G8B8A8_UNORM; break;
        default: printf("ChannelCount %d not support.\n", channelCount); return TANTO_S_NONE;
    }

    tanto_v_LoadImage(filePath, channelCount, format,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_IMAGE_ASPECT_COLOR_BIT, 
            1, VK_FILTER_LINEAR, tanto_v_GetQueueFamilyIndex(TANTO_V_QUEUE_GRAPHICS_TYPE), 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, &texture.devImage);

    const Tanto_S_TextureId texId = scene->textureCount++;
    scene->textures[texId] = texture;
    assert(texId < TANTO_S_MAX_TEXTURES);

    scene->dirt |= TANTO_S_TEXTURES_BIT;

    return texId;
}

Tanto_S_MaterialId tanto_s_CreateMaterial(Tanto_S_Scene* scene, Vec3 color, float roughness, Tanto_S_TextureId albedoId, Tanto_S_TextureId roughnessId)
{
    Tanto_S_MaterialId matId = scene->materialCount++;
    assert(matId < TANTO_S_MAX_TEXTURES);

    scene->materials[matId].color = color;
    scene->materials[matId].roughness = roughness;
    scene->materials[matId].textureAlbedo = albedoId;
    scene->materials[matId].textureRoughness = roughnessId;

    scene->dirt |= TANTO_S_MATERIALS_BIT;

    return matId;
}

Tanto_S_LightId tanto_s_CreateDirectionLight(Scene* scene, const Vec3 color, const Vec3 direction)
{
    Light light = {
        .color = color,
        .intensity = 1,
        .type = TANTO_S_LIGHT_TYPE_DIRECTION,
        .structure.directionLight.dir = m_Normalize_Vec3(&direction)
    };

    const uint32_t curIndex = scene->lightCount++;
    assert(curIndex < TANTO_S_MAX_LIGHTS);
    scene->lights[curIndex] = light;
    scene->dirt |= TANTO_S_LIGHTS_BIT;
    return curIndex;
}

Tanto_S_LightId tanto_s_CreatePointLight(Scene* scene, const Vec3 color, const Vec3 position)
{
    Light light = {
        .color = color,
        .intensity = 1,
        .type = TANTO_S_LIGHT_TYPE_POINT,
        .structure.pointLight.pos = position
    };

    const uint32_t curIndex = scene->lightCount++;
    assert(curIndex < TANTO_S_MAX_LIGHTS);
    scene->lights[curIndex] = light;
    scene->dirt |= TANTO_S_LIGHTS_BIT;
    return curIndex;
}

#define HOME_POS    {0.0, 0.0, 1.0}
#define HOME_TARGET {0.0, 0.0, 0.0}
#define HOME_UP     {0.0, 1.0, 0.0}
#define ZOOM_RATE   0.005
#define PAN_RATE    0.1
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
