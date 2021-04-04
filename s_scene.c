#include "s_scene.h"
#include "coal/m.h"
#include "f_file.h"
#include "coal/m_math.h"
#include "coal/util.h"
#include "hell/common.h"
#include "obsidian/v_memory.h"
#include "r_geo.h"
#include "v_image.h"
#include <string.h>
#define ARCBALL_CAMERA_IMPLEMENTATION
#include "arcball_camera.h"

typedef Obdn_R_Primitive     Primitive;
typedef Obdn_S_Xform         Xform;
typedef Obdn_S_Scene         Scene;
typedef Obdn_S_Light         Light;
typedef Obdn_S_Material      Material;
typedef Obdn_S_Camera        Camera;
typedef Obdn_S_Texture       Texture;
typedef Obdn_S_LightId       LightId;

typedef Obdn_S_PrimitiveList PrimitiveList;

// TODO: this function needs to updated
#define DEFAULT_MAT_ID 0
#define MAX_DONOR_PRIM_IDS 16

static Obdn_S_PrimId donorPrimIds[MAX_DONOR_PRIM_IDS];
static uint8_t       donorPrimIdCount;

// lightMap is an indirection from Light Id to the actual light data in the scene.
// invariants are that the active lights in the scene are tightly packed and
// that the lights in the scene are ordered such that their indices are ordered
// within the map.
typedef int32_t LightIndex;
static LightIndex lightMap[OBDN_S_MAX_LIGHTS]; // maps light id to light index
static LightId    nextLightId = 0; // this always increases


static Obdn_S_PrimId addPrim(Scene* scene, const Obdn_R_Primitive rprim, const Coal_Mat4 xform)
{
    uint32_t curIndex;
    if (donorPrimIdCount)
        curIndex = donorPrimIds[--donorPrimIdCount];
    else
        curIndex = scene->primCount++;
    assert(curIndex < OBDN_S_MAX_PRIMS);
    if (xform)
        coal_Copy_Mat4(xform, scene->xforms[curIndex]);
    scene->prims[curIndex].rprim = rprim;
    scene->prims[curIndex].inactive = false;
    scene->dirt |= OBDN_S_XFORMS_BIT | OBDN_S_PRIMS_BIT;
    return curIndex;
}

static void removePrim(Scene* s, Obdn_S_PrimId id)
{
    if (s->prims[id].inactive) return;
    s->prims[id].inactive = true;
    s->dirt |= OBDN_S_PRIMS_BIT;
    obdn_r_FreePrim(&s->prims[id].rprim);
    donorPrimIds[donorPrimIdCount++] = id;
}

static LightId addLight(Scene* s, Light light)
{
    hell_Print("Adding light...\nBefore info:\n");
    obdn_s_PrintLightInfo(s);
    LightIndex index  = s->lightCount++;
    LightId    id     = nextLightId++;
    while (lightMap[id % OBDN_S_MAX_LIGHTS] >= 0) 
        id = nextLightId++;
    LightId    slot   = id % OBDN_S_MAX_LIGHTS;
    printf("Index: %d slot %d\n", index, slot);
    if (index > slot)
    {
        memmove(s->lights + slot + 1, s->lights + slot, sizeof(Light) * (s->lightCount - (slot + 1)));
        for (int i = slot + 1; i < OBDN_S_MAX_LIGHTS; i++)
        {
            ++lightMap[i];
        }
    }
    index = slot;
    lightMap[slot] = index;
    memcpy(&s->lights[index], &light, sizeof(Light));

    s->dirt |= OBDN_S_LIGHTS_BIT;
    hell_Print("After info: \n");
    obdn_s_PrintLightInfo(s);
    return id;
}

static LightId addDirectionLight(Scene* s, const Coal_Vec3 dir, const Coal_Vec3 color, const float intensity)
{
    Light light = {
        .type = OBDN_S_LIGHT_TYPE_DIRECTION,
        .intensity = intensity
    };
    coal_Copy_Vec3(dir, light.structure.directionLight.dir.x);
    coal_Copy_Vec3(color, light.color.x);
    return addLight(s, light);
}

static LightId addPointLight(Scene* s, const Coal_Vec3 pos, const Coal_Vec3 color, const float intensity)
{
    Light light = {
        .type = OBDN_S_LIGHT_TYPE_POINT,
        .intensity = intensity
    };
    coal_Copy_Vec3(pos, light.structure.pointLight.pos.x);
    coal_Copy_Vec3(color, light.color.x);
    return addLight(s, light);
}

static void removeLight(Scene* s, LightId id)
{
    LightId slot = id % OBDN_S_MAX_PRIMS;
    LightIndex dst = lightMap[slot];
    LightIndex src = dst + 1;
    memmove(s->lights + dst, s->lights + src, sizeof(Light) * (s->lightCount - src));
    s->lightCount--;
    lightMap[slot] = 0;
    for (int i = slot; i < OBDN_S_MAX_LIGHTS; i++)
    {
        --lightMap[i];
    }
    s->dirt |= OBDN_S_LIGHTS_BIT;
}

void obdn_s_Init(Scene* scene, uint16_t windowWidth, uint16_t windowHeight, float nearClip, float farClip)
{
    memset(scene, 0, sizeof(Scene));
    scene->window[0] = windowWidth;
    scene->window[1] = windowHeight;
    scene->camera.xform = m_Ident_Mat4();
    scene->camera.view = m_Ident_Mat4();
    Mat4 m = m_LookAt(&(Vec3){1, 1, 2}, &(Vec3){0, 0, 0}, &(Vec3){0, 1, 0});
    scene->camera.xform = m;
    scene->camera.view = m_Invert4x4(&m);
    scene->camera.proj = m_BuildPerspective(nearClip, farClip);
    // set all xforms to identity
    for (int i = 0; i < OBDN_S_MAX_PRIMS; i++) 
    {
        coal_Mat4_Identity(scene->xforms[i]);
    }
    obdn_s_CreateMaterial(scene, (Vec3){0, 0.937, 1.0}, 0.8, 0, 0, 0); // default. color is H-Beta from hydrogen balmer series
    for (int i = 0; i < OBDN_S_MAX_LIGHTS; i++)
    {
        lightMap[i] = -1;
    }

    scene->dirt = -1;
}

void obdn_s_CreateEmptyScene(Scene* scene)
{
    memset(scene, 0, sizeof(Scene));
    Mat4 m = m_LookAt(&(Vec3){1, 1, 2}, &(Vec3){0, 0, 0}, &(Vec3){0, 1, 0});
    scene->camera.xform = m;
    scene->camera.view = m_Invert4x4(&m);
    scene->camera.proj = m_BuildPerspective(0.01, 100);
    obdn_s_CreateMaterial(scene, (Vec3){1, .4, .7}, 1, 0, 0, 0); // default matId
    for (int i = 0; i < OBDN_S_MAX_PRIMS; i++) 
    {
        coal_Mat4_Identity(scene->xforms[i]);
    }

    scene->dirt |= -1;
}

void obdn_s_BindPrimToMaterial(Scene* scene, const Obdn_S_PrimId primId, const Obdn_S_MaterialId matId)
{
    assert(scene->materialCount > matId);
    assert(scene->primCount > primId);
    scene->prims[primId].materialId = matId;

    scene->dirt |= OBDN_S_PRIMS_BIT;
}

Obdn_S_PrimId obdn_s_AddRPrim(Scene* scene, const Obdn_R_Primitive prim, const Coal_Mat4 xform)
{
    return addPrim(scene, prim, xform);
}

Obdn_S_PrimId obdn_s_LoadPrim(Scene* scene, const char* filePath, const Coal_Mat4 xform)
{
    Obdn_F_Primitive fprim;
    int r = obdn_f_ReadPrimitive(filePath, &fprim);
    assert(r);
    Obdn_R_Primitive prim = obdn_f_CreateRPrimFromFPrim(&fprim);
    obdn_r_TransferPrimToDevice(&prim);
    obdn_f_FreePrimitive(&fprim);
    return addPrim(scene, prim, xform);
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
            1, VK_FILTER_LINEAR, OBDN_V_MEMORY_DEVICE_TYPE, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, &texture.devImage);

    const Obdn_S_TextureId texId = ++scene->textureCount; // we purposely leave the 0th texture slot empty
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
    return addDirectionLight(scene, direction.x, color.x, 1.0);
}

Obdn_S_LightId obdn_s_CreatePointLight(Scene* scene, const Vec3 color, const Vec3 position)
{
    return addPointLight(scene, position.x, color.x, 1.0);
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
    scene->camera.view  = m_Invert4x4(&scene->camera.xform);
    scene->dirt |= OBDN_S_CAMERA_VIEW_BIT;
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
            ZOOM_RATE, PAN_RATE, TUMBLE_RATE, scene->window[0], scene->window[1], xPrev, mx, yPrev, my, 
            panning, tumbling, zoom_ticks, 0);
    Mat4 m = m_LookAt(&pos, &target, &up);
    scene->camera.xform = m;
    scene->camera.view  = m_Invert4x4(&scene->camera.xform);
    xPrev = mx;
    yPrev = my;
    scene->dirt |= OBDN_S_CAMERA_VIEW_BIT;
}

void obdn_s_UpdateLight(Scene* scene, uint32_t id, float intensity)
{
    scene->lights[lightMap[id]].intensity = intensity;
    scene->dirt |= OBDN_S_LIGHTS_BIT;
}

void obdn_s_UpdatePrimXform(Scene* scene, const Obdn_S_PrimId primId, const Mat4* delta)
{
    assert(primId < scene->primCount);
    Coal_Mat4 M;
    coal_Mult_Mat4(scene->xforms[primId], delta->x, M);
    coal_Copy_Mat4(M, scene->xforms[primId]);
    scene->dirt |= OBDN_S_XFORMS_BIT;
}

void obdn_s_AddPrimToList(const Obdn_S_PrimId primId, Obdn_S_PrimitiveList* list)
{
    list->primIds[list->primCount] = primId;
    list->primCount++;
    assert(list->primCount < OBDN_S_MAX_PRIMS);
}

void obdn_s_ClearPrimList(Obdn_S_PrimitiveList* list)
{
    list->primCount = 0;
    assert(list->primCount < OBDN_S_MAX_PRIMS);
}

void obdn_s_CleanUpScene(Obdn_S_Scene* scene)
{
    for (int i = 0; i < scene->primCount; i++)
    {
        if (scene->prims[i].inactive) continue;
        obdn_r_FreePrim(&scene->prims[i].rprim);
    }
    for (int i = 1; i <= scene->textureCount; i++) // remember 1 is the first valid texture index
    {
        obdn_v_FreeImage(&scene->textures[i].devImage);   
        if (scene->textures[i].hostBuffer.hostData)
            obdn_v_FreeBufferRegion(&scene->textures[i].hostBuffer);
    }
    memset(scene, 0, sizeof(*scene));
}

Obdn_R_Primitive obdn_s_SwapRPrim(Obdn_S_Scene* scene, const Obdn_R_Primitive* newRprim, const Obdn_S_PrimId id)
{
    assert(id < scene->primCount);
    Obdn_R_Primitive oldPrim = scene->prims[id].rprim;
    scene->prims[id].rprim = *newRprim;
    scene->dirt |= OBDN_S_PRIMS_BIT;
    return oldPrim;
}

bool obdn_s_PrimExists(const Obdn_S_Scene* s, Obdn_S_PrimId id)
{
    return (s->primCount > 0 && s->prims[id].rprim.attrCount > 0);
}

void obdn_s_RemovePrim(Obdn_S_Scene* s, Obdn_S_PrimId id)
{
    removePrim(s, id);
}

void obdn_s_AddPointLight(Scene* s, Coal_Vec3 pos, Coal_Vec3 color, float intensity)
{
    addPointLight(s, pos, color, intensity);
}

void obdn_s_RemoveLight(Scene* s, LightId id)
{
    removeLight(s, id);
}

void obdn_s_PrintLightInfo(const Scene* s)
{
    hell_Print("====== Scene: light info =======\n");
    hell_Print("Light count: %d\n", s->lightCount);
    hell_Print("Light map: ");
    for (int i = 0; i < OBDN_S_MAX_LIGHTS; i++)
    {
        hell_Print(" %d:%d ", i, lightMap[i]);
    }
    hell_Print("\n");
}
