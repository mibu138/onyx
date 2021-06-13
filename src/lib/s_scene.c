#include "s_scene.h"
#include "f_file.h"
#include "coal/util.h"
#include "hell/common.h"
#include "dtags.h"
#include <hell/debug.h>
#include "v_memory.h"
#include "common.h"
#include "r_geo.h"
#include "v_image.h"
#include <string.h>
#define ARCBALL_CAMERA_IMPLEMENTATION
#include "arcball_camera.h"

typedef Obdn_Primitive     Primitive;
typedef Obdn_Xform         Xform;
typedef Obdn_Scene         Scene;
typedef Obdn_Light         Light;
typedef Obdn_Material      Material;
typedef Obdn_Camera        Camera;
typedef Obdn_Texture       Texture;

typedef Obdn_LightId       LightId;
typedef Obdn_PrimId        PrimId;

typedef Obdn_PrimitiveList PrimitiveList;

// TODO: this function needs to updated
#define DEFAULT_MAT_ID 0

#define DPRINT(fmt, ...) hell_DebugPrint(OBDN_DEBUG_TAG_SCENE, fmt, ##__VA_ARGS__)

typedef int32_t LightIndex;
typedef int32_t PrimIndex;
// lightMap is an indirection from Light Id to the actual light data in the scene.
// invariants are that the active lights in the scene are tightly packed and
// that the lights in the scene are ordered such that their indices are ordered
// within the map.
static struct {
    LightIndex indices[OBDN_SCENE_MAX_LIGHTS]; // maps light id to light index
    LightId    nextId; // this always increases
} lightMap;

static struct {
    PrimIndex indices[OBDN_SCENE_MAX_PRIMS];
    PrimId    nextId;
} primMap;

static Obdn_PrimId addPrim(Scene* s, const Obdn_Primitive prim)
{
    PrimIndex index  = s->primCount++;
    assert(index < OBDN_SCENE_MAX_PRIMS);
    PrimId    id     = primMap.nextId++;
    while (primMap.indices[id % OBDN_SCENE_MAX_PRIMS] >= 0) 
        id = primMap.nextId++;
    PrimId    slot   = id % OBDN_SCENE_MAX_PRIMS;
    if (index > slot)
    {
        memmove(s->prims + slot + 1, s->prims + slot, sizeof(Primitive) * (s->primCount - (slot + 1)));
        for (int i = slot + 1; i < OBDN_SCENE_MAX_PRIMS; i++)
        {
            ++primMap.indices[i];
        }
        index = slot;
    }
    primMap.indices[slot] = index;
    memcpy(&s->prims[index], &prim, sizeof(Primitive));

    s->dirt |= OBDN_SCENE_PRIMS_BIT;
    return id;
}

static void removePrim(Scene* s, Obdn_PrimId id)
{
    PrimId slot = id % OBDN_SCENE_MAX_PRIMS;
    PrimIndex dst = primMap.indices[slot];
    PrimIndex src = dst + 1;
    obdn_FreeGeo(&s->prims[dst].geo);
    memmove(s->prims + dst, s->prims + src, sizeof(Primitive) * (s->primCount - src));
    s->primCount--;
    for (int i = slot + 1; i < OBDN_SCENE_MAX_PRIMS; i++)
    {
        --primMap.indices[i];
    }
    primMap.indices[slot] = -OBDN_SCENE_MAX_PRIMS;
    s->dirt |= OBDN_SCENE_PRIMS_BIT;
}

static LightId addLight(Scene* s, Light light)
{
    DPRINT("Adding light...\nBefore info:\n");
    obdn_PrintLightInfo(s);
    LightIndex index  = s->lightCount++;
    assert(index < OBDN_SCENE_MAX_LIGHTS);
    LightId    id     = lightMap.nextId++;
    while (lightMap.indices[id % OBDN_SCENE_MAX_LIGHTS] >= 0) 
        id = lightMap.nextId++;
    LightId    slot   = id % OBDN_SCENE_MAX_LIGHTS;
    DPRINT("Index: %d slot %d\n", index, slot);
    if (index > slot)
    {
        memmove(s->lights + slot + 1, s->lights + slot, sizeof(Light) * (s->lightCount - (slot + 1)));
        for (int i = slot + 1; i < OBDN_SCENE_MAX_LIGHTS; i++)
        {
            ++lightMap.indices[i];
        }
        index = slot;
    }
    lightMap.indices[slot] = index;
    memcpy(&s->lights[index], &light, sizeof(Light));

    s->dirt |= OBDN_SCENE_LIGHTS_BIT;
    DPRINT("After info: \n");
    obdn_PrintLightInfo(s);
    return id;
}

static LightId addDirectionLight(Scene* s, const Coal_Vec3 dir, const Coal_Vec3 color, const float intensity)
{
    Light light = {
        .type = OBDN_DIRECTION_LIGHT_TYPE,
        .intensity = intensity,
    };
    light.structure.directionLight.dir = dir;
    light.color = color;
    return addLight(s, light);
}

static LightId addPointLight(Scene* s, const Coal_Vec3 pos, const Coal_Vec3 color, const float intensity)
{
    Light light = {
        .type = OBDN_LIGHT_POINT_TYPE,
        .intensity = intensity
    };
    light.structure.pointLight.pos = pos;
    light.color = color;
    return addLight(s, light);
}

static void removeLight(Scene* s, LightId id)
{
    LightId slot = id % OBDN_SCENE_MAX_PRIMS;
    LightIndex dst = lightMap.indices[slot];
    LightIndex src = dst + 1;
    memmove(s->lights + dst, s->lights + src, sizeof(Light) * (s->lightCount - src));
    s->lightCount--;
    for (int i = slot + 1; i < OBDN_SCENE_MAX_LIGHTS; i++)
    {
        --lightMap.indices[i];
    }
    lightMap.indices[slot] = -OBDN_SCENE_MAX_LIGHTS;
    s->dirt |= OBDN_SCENE_LIGHTS_BIT;
}

void obdn_CreateScene(uint16_t windowWidth, uint16_t windowHeight, float nearClip, float farClip, Scene* scene)
{
    memset(scene, 0, sizeof(Scene));
    scene->camera.xform = coal_Ident_Mat4();
    scene->camera.view = coal_Ident_Mat4();
    Mat4 m = coal_LookAt((Vec3){1, 1, 2}, (Vec3){0, 0, 0}, (Vec3){0, 1, 0});
    scene->camera.xform = m;
    scene->camera.view = coal_Invert4x4(m);
    scene->camera.proj = coal_BuildPerspective(nearClip, farClip);
    // set all xforms to identity
    obdn_CreateMaterial(scene, (Vec3){0, 0.937, 1.0}, 0.8, 0, 0, 0); // default. color is H-Beta from hydrogen balmer series
    for (int i = 0; i < OBDN_SCENE_MAX_LIGHTS; i++)
    {
        lightMap.indices[i] = -OBDN_SCENE_MAX_LIGHTS;
    }
    for (int i = 0; i < OBDN_SCENE_MAX_PRIMS; i++)
    {
        primMap.indices[i] = -OBDN_SCENE_MAX_PRIMS;
    }

    scene->dirt = -1;
}

void obdn_CreateEmptyScene(Scene* scene)
{
    memset(scene, 0, sizeof(Scene));
    Mat4 m = coal_LookAt((Vec3){1, 1, 2}, (Vec3){0, 0, 0}, (Vec3){0, 1, 0});
    scene->camera.xform = m;
    scene->camera.view = coal_Invert4x4(m);
    scene->camera.proj = coal_BuildPerspective(0.01, 100);
    obdn_CreateMaterial(scene, (Vec3){1, .4, .7}, 1, 0, 0, 0); // default matId

    scene->dirt |= -1;
}

void obdn_BindPrimToMaterial(Scene* scene, const Obdn_PrimId primId, const Obdn_MaterialId matId)
{
    assert(scene->materialCount > matId);
    scene->prims[primMap.indices[primId]].materialId = matId;

    scene->dirt |= OBDN_SCENE_PRIMS_BIT;
}

Obdn_PrimId obdn_AddPrim(Scene* scene, const Obdn_Geometry geo, const Coal_Mat4 xform)
{
    Obdn_Primitive prim = {
        .geo = geo,
        .xform = COAL_MAT4_IDENT,
        .materialId = 0
    };
    prim.xform = xform;
    return addPrim(scene, prim);
}

Obdn_PrimId obdn_LoadPrim(Scene* scene, Obdn_Memory* memory, const char* filePath, const Coal_Mat4 xform)
{
    Obdn_FileGeo fprim;
    int r = obdn_ReadFileGeo(filePath, &fprim);
    assert(r);
    Obdn_Geometry prim = obdn_CreateGeoFromFileGeo(memory, &fprim);
    obdn_TransferGeoToDevice(memory, &prim);
    obdn_FreeFileGeo(&fprim);
    obdn_Announce("Loaded prim at %s\n", filePath);
    return obdn_AddPrim(scene, prim, xform);
}

Obdn_TextureId obdn_LoadTexture(Obdn_Scene* scene, Obdn_Memory* memory, const char* filePath, const uint8_t channelCount)
{
    Texture texture = {0};

    VkFormat format;

    switch (channelCount) 
    {
        case 1: format = VK_FORMAT_R8_UNORM; break;
        case 3: format = VK_FORMAT_R8G8B8A8_UNORM; break;
        case 4: format = VK_FORMAT_R8G8B8A8_UNORM; break;
        default: DPRINT("ChannelCount %d not support.\n", channelCount); return 0;
    }

    obdn_LoadImage(memory, filePath, channelCount, format,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_IMAGE_ASPECT_COLOR_BIT, 
            1, VK_FILTER_LINEAR, OBDN_V_MEMORY_DEVICE_TYPE, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, &texture.devImage);

    const Obdn_TextureId texId = ++scene->textureCount; // we purposely leave the 0th texture slot empty
    scene->textures[texId] = texture;
    assert(scene->textureCount < OBDN_SCENE_MAX_TEXTURES);

    scene->dirt |= OBDN_SCENE_TEXTURES_BIT;

    return texId;
}

Obdn_MaterialId obdn_CreateMaterial(Obdn_Scene* scene, Vec3 color, float roughness, 
        Obdn_TextureId albedoId, Obdn_TextureId roughnessId, Obdn_TextureId normalId)
{
    Obdn_MaterialId matId = scene->materialCount++;
    assert(matId < OBDN_SCENE_MAX_MATERIALS);

    scene->materials[matId].color     = color;
    scene->materials[matId].roughness = roughness;
    scene->materials[matId].textureAlbedo    = albedoId;
    scene->materials[matId].textureRoughness = roughnessId;
    scene->materials[matId].textureNormal    = normalId;

    scene->dirt |= OBDN_SCENE_MATERIALS_BIT;

    return matId;
}

Obdn_LightId obdn_CreateDirectionLight(Scene* scene, const Vec3 color, const Vec3 direction)
{
    return addDirectionLight(scene, direction, color, 1.0);
}

Obdn_LightId obdn_SceneCreatePointLight(Scene* scene, const Vec3 color, const Vec3 position)
{
    return addPointLight(scene, position, color, 1.0);
}

#define HOME_POS    {0.0, 0.0, 1.0}
#define HOME_TARGET {0.0, 0.0, 0.0}
#define HOME_UP     {0.0, 1.0, 0.0}
#define ZOOM_RATE   0.005
#define PAN_RATE    0.1
#define TUMBLE_RATE 2

void obdn_UpdateCamera_LookAt(Scene* scene, Vec3 pos, Vec3 target, Vec3 up)
{
    scene->camera.xform = coal_LookAt(pos, target, up);
    scene->camera.view  = coal_Invert4x4(scene->camera.xform);
    scene->dirt |= OBDN_SCENE_CAMERA_VIEW_BIT;
}

void obdn_UpdateCamera_ArcBall(Scene* scene, int screenWidth, int screenHeight, float dt, int xprev, int x, int yprev, int y, bool panning, bool tumbling, bool zooming, bool home)
{
    Vec3 pos    = HOME_POS;
    Vec3 target = HOME_TARGET;
    Vec3 up     = HOME_UP;
    int zoom_ticks = 0;
    if (zooming)
        zoom_ticks = x - xprev;
    else
        zoom_ticks = 0;
    if (home)
    {
        pos =    (Vec3)HOME_POS;
        target = (Vec3)HOME_TARGET;
        up =     (Vec3)HOME_UP;
    }
    //pos = m_RotateY_Vec3(dt, &pos);
    arcball_camera_update(pos.e, target.e, up.e, NULL, dt, 
            ZOOM_RATE, PAN_RATE, TUMBLE_RATE, screenWidth, screenHeight, xprev, x, yprev, y, 
            panning, tumbling, zoom_ticks, 0);
    Mat4 m = coal_LookAt(pos, target, up);
    coal_PrintMat4(m);
    scene->camera.xform = m;
    scene->camera.view  = coal_Invert4x4(scene->camera.xform);
    scene->dirt |= OBDN_SCENE_CAMERA_VIEW_BIT;
}

void obdn_UpdateLight(Scene* scene, uint32_t id, float intensity)
{
    scene->lights[lightMap.indices[id]].intensity = intensity;
    scene->dirt |= OBDN_SCENE_LIGHTS_BIT;
}

void obdn_UpdatePrimXform(Scene* scene, const Obdn_PrimId primId, const Mat4 delta)
{
    assert(primId < scene->primCount);
    Coal_Mat4 M = coal_Mult_Mat4(scene->prims[primMap.indices[primId]].xform, delta);
    scene->prims[primMap.indices[primId]].xform = M;
    scene->dirt |= OBDN_SCENE_XFORMS_BIT;
}

void obdn_AddPrimToList(const Obdn_PrimId primId, Obdn_PrimitiveList* list)
{
    list->primIds[list->primCount] = primId;
    list->primCount++;
    assert(list->primCount < OBDN_SCENE_MAX_PRIMS);
}

void obdn_ClearPrimList(Obdn_PrimitiveList* list)
{
    list->primCount = 0;
    assert(list->primCount < OBDN_SCENE_MAX_PRIMS);
}

void obdn_CleanUpScene(Obdn_Scene* scene)
{
    for (int i = 0; i < scene->primCount; i++)
    {
        obdn_FreeGeo(&scene->prims[i].geo);
    }
    for (int i = 1; i <= scene->textureCount; i++) // remember 1 is the first valid texture index
    {
        obdn_FreeImage(&scene->textures[i].devImage);   
        if (scene->textures[i].hostBuffer.hostData)
            obdn_FreeBufferRegion(&scene->textures[i].hostBuffer);
    }
    memset(scene, 0, sizeof(*scene));
}

Obdn_Geometry obdn_SwapRPrim(Obdn_Scene* scene, const Obdn_Geometry* newRprim, const Obdn_PrimId id)
{
    assert(id < scene->primCount);
    Obdn_Geometry oldPrim = scene->prims[id].geo;
    scene->prims[id].geo = *newRprim;
    scene->dirt |= OBDN_SCENE_PRIMS_BIT;
    return oldPrim;
}

bool obdn_PrimExists(const Obdn_Scene* s, Obdn_PrimId id)
{
    return (s->primCount > 0 && s->prims[id].geo.attrCount > 0);
}

void obdn_RemovePrim(Obdn_Scene* s, Obdn_PrimId id)
{
    removePrim(s, id);
}

void obdn_SceneAddDirectionLight(Scene* s, Coal_Vec3 dir, Coal_Vec3 color, float intensity)
{
    addDirectionLight(s, dir, color, intensity);
}

void obdn_SceneAddPointLight(Scene* s, Coal_Vec3 pos, Coal_Vec3 color, float intensity)
{
    addPointLight(s, pos, color, intensity);
}

void obdn_RemoveLight(Scene* s, LightId id)
{
    removeLight(s, id);
}

void obdn_PrintLightInfo(const Scene* s)
{
    hell_Print("====== Scene: light info =======\n");
    hell_Print("Light count: %d\n", s->lightCount);
    for (int i = 0; i < s->lightCount; i++)
    {
        hell_Print("Light index %d P ", i);
        hell_Print_Vec3(s->lights[i].structure.pointLight.pos.e);
        hell_Print(" C ");
        hell_Print_Vec3(s->lights[i].color.e);
        hell_Print(" I  %f\n", s->lights[i].intensity);
    }
    hell_Print("Light map: ");
    for (int i = 0; i < OBDN_SCENE_MAX_LIGHTS; i++)
    {
        hell_Print(" %d:%d ", i, lightMap.indices[i]);
    }
    hell_Print("\n");
}

void obdn_PrintPrimInfo(const Scene* s)
{
    hell_Print("====== Scene: primitive info =======\n");
    hell_Print("Prim count: %d\n", s->primCount);
    for (int i = 0; i < s->primCount; i++)
    {
        hell_Print("Prim %d material id %d\n", i, s->prims[i].materialId); 
        hell_Print("Material: id %d roughness %f\n", s->prims[i].materialId, s->materials[s->prims[i].materialId].roughness);
        hell_Print_Mat4(s->prims[i].xform.e);
        hell_Print("\n");
    }
    hell_Print("Prim map: ");
    for (int i = 0; i < OBDN_SCENE_MAX_PRIMS; i++)
    {
        hell_Print(" %d:%d ", i, primMap.indices[i]);
    }
    hell_Print("\n");
}

void obdn_UpdateLightColor(Obdn_Scene* scene, Obdn_LightId id, float r, float g, float b)
{
    scene->lights[lightMap.indices[id]].color.r = r;
    scene->lights[lightMap.indices[id]].color.g = g;
    scene->lights[lightMap.indices[id]].color.b = b;
    scene->dirt |= OBDN_SCENE_LIGHTS_BIT;
}

void obdn_UpdateLightPos(Obdn_Scene* scene, Obdn_LightId id, float x, float y, float z)
{
    scene->lights[lightMap.indices[id]].structure.pointLight.pos.x = x;
    scene->lights[lightMap.indices[id]].structure.pointLight.pos.y = y;
    scene->lights[lightMap.indices[id]].structure.pointLight.pos.z = z;
    scene->dirt |= OBDN_SCENE_LIGHTS_BIT;
}

void obdn_UpdateLightIntensity(Obdn_Scene* scene, Obdn_LightId id, float i)
{
    scene->lights[lightMap.indices[id]].intensity = i;
    scene->dirt |= OBDN_SCENE_LIGHTS_BIT;
}

Obdn_Scene* obdn_AllocScene(void)
{
    return hell_Malloc(sizeof(Scene));
}
