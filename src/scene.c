#define COAL_SIMPLE_TYPE_NAMES
#include "scene.h"
#include "file.h"
#include "coal/util.h"
#include "coal/linalg.h"
#include "hell/common.h"
#include <hell/ds.h>
#include "dtags.h"
#include <hell/debug.h>
#include "memory.h"
#include "common.h"
#include "geo.h"
#include "image.h"
#include <string.h>
#define ARCBALL_CAMERA_IMPLEMENTATION
#include "arcball_camera.h"

typedef Onyx_Primitive     Primitive;
typedef Onyx_Xform         Xform;
typedef Onyx_Scene         Scene;
typedef Onyx_Light         Light;
typedef Onyx_Material      Material;
typedef Onyx_Camera        Camera;
typedef Onyx_Texture       Texture;

typedef Onyx_SceneObjectInt obint;

typedef Onyx_PrimitiveHandle PrimitiveHandle;
typedef Onyx_LightHandle LightHandle;
typedef Onyx_MaterialHandle MaterialHandle;
typedef Onyx_TextureHandle TextureHandle;

typedef Onyx_PrimitiveList PrimitiveList;

#define INIT_PRIM_CAP 16
#define INIT_LIGHT_CAP 8
#define INIT_MATERIAL_CAP 8
#define INIT_TEXTURE_CAP 8

#define PRIM(scene, handle) scene->prims[scene->primMap.indices[handle.id]]
#define TEXTURE(scene, handle) scene->textures[scene->texMap.indices[handle.id]]
#define LIGHT(scene, handle) scene->lights[scene->lightMap.indices[handle.id]]
#define MATERIAL(scene, handle) scene->materials[scene->matMap.indices[handle.id]]

#define DPRINT(fmt, ...) hell_DebugPrint(ONYX_DEBUG_TAG_SCENE, fmt, ##__VA_ARGS__)

// lightMap is an indirection from Light Id to the actual light data in the scene.
// invariants are that the active lights in the scene are tightly packed and
// that the lights in the scene are ordered such that their indices are ordered
// within the map.

typedef struct {
    // an array of indices into the object buffers. a handle id is an index into
    // this.
    obint*  indices;
    // a stack of ids that are available for reuse. gets added to when we remove
    // an object and can reclaim its id. the bottom of the stack should always
    // be larger than any other Id used yet. in other words, we should always pull from this
    // stack for the next id
    Hell_Array availableIds;
} ObjectMap;

typedef struct Onyx_Scene {
    Onyx_SceneDirtyFlags dirt;
    Onyx_Memory*         memory;
    Hell_Array           dirtyTextures;
    Hell_Array           dirtyMaterials;
    Hell_Array           dirtyPrims;
    obint                primCount;
    obint                lightCount;
    obint                materialCount;
    obint                textureCount;
    Onyx_Camera          camera;
    obint                primCapacity;
    Onyx_Primitive*      prims;
    obint                materialCapacity;
    Onyx_Material*       materials;
    obint                textureCapacity;
    Onyx_Texture*        textures;
    obint                lightCapacity;
    Onyx_Light*          lights;
    Onyx_Geometry        defaultGeo;
    Onyx_Image           defaultImage;
    ObjectMap            primMap;
    ObjectMap            lightMap;
    ObjectMap            matMap;
    ObjectMap            texMap;
} Onyx_Scene;

static void addTextureToDirtyTextures(Onyx_Scene* s, TextureHandle handle)
{
    TextureHandle* texs = s->dirtyTextures.elems;
    for (int i = 0; i < s->dirtyTextures.count; i++)
    {
        if (texs[i].id == handle.id) return; // handle already in set
    }
    hell_ArrayPush(&s->dirtyTextures, &handle);
}

static void addMaterialToDirtyMaterials(Onyx_Scene* s, MaterialHandle handle)
{
    MaterialHandle* mats = s->dirtyMaterials.elems;
    for (int i = 0; i < s->dirtyMaterials.count; i++)
    {
        if (mats[i].id == handle.id) return; // handle already in set
    }
    hell_ArrayPush(&s->dirtyMaterials, &handle);
}

static void addPrimToDirtyPrims(Onyx_Scene* s, PrimitiveHandle handle)
{
    PrimitiveHandle* prims = s->dirtyPrims.elems;
    for (int i = 0; i < s->dirtyPrims.count; i++)
    {
        if (prims[i].id == handle.id) return; // handle already in set
    }
    hell_ArrayPush(&s->dirtyPrims, &handle);
}

static void createObjectMap(u32 initObjectCap, u32 initIdStackCap, ObjectMap* map)
{
    map->indices = hell_Malloc(sizeof(map->indices[0]) * initObjectCap);
    hell_CreateArray(initIdStackCap, sizeof(map->indices[0]), NULL, NULL, &map->availableIds);
}

static void growArray(void** curptr, obint* curcap, const u32 elemsize)
{
    assert(*curcap);
    assert(*curptr);
    uint32_t newcap = *curcap * 2;
    void* p = hell_Realloc(*curptr, newcap * elemsize);
    if (!p)
        hell_Error(HELL_ERR_FATAL, "Growing array capacity failed. Realloc returned Null\n");
    *curptr = p;
    *curcap = newcap;
}

static obint addSceneObject(const void* object, void* objectArray, obint* objectCount, obint* cap, const u32 elemSize, ObjectMap* map)
{
    const obint index = (*objectCount)++;
    if (index == *cap)
    {
        growArray(objectArray, cap, elemSize);
        growArray((void**)&map->indices, cap, elemSize);
    }
    assert(index < *cap);
    obint id = 0;
    if (map->availableIds.count == 0)
        id = index;
    else
        hell_ArrayPop(&map->availableIds, &id);
    map->indices[id] = index;
    void* dst = (u8*)(objectArray) + index * elemSize;
    memcpy(dst, object, elemSize);
    return id;
}

static void removeSceneObject(obint id, void* objectArray, obint* objectCount, const u32 elemSize, ObjectMap* map)
{
    void* const       dst   = (u8*)(objectArray) + map->indices[id] * elemSize;
    const void* const src   = (u8*)dst + elemSize;
    const u8* const   end   = (u8*)(objectArray) + *objectCount * elemSize;
    const u32         range = end - (u8*)src;
    memmove(dst, src, range);
    (*objectCount)--;
    hell_ArrayPush(&map->availableIds, &id);
}

static PrimitiveHandle addPrim(Scene* s, Onyx_Primitive prim)
{
    obint id = addSceneObject(&prim, s->prims, &s->primCount, &s->primCapacity, sizeof(prim), &s->primMap);
    PrimitiveHandle handle = {id};
    addPrimToDirtyPrims(s, handle);
    PRIM(s, handle).dirt |= ONYX_PRIM_ADDED_BIT;
    s->dirt |= ONYX_SCENE_PRIMS_BIT;
    return handle;
}

static LightHandle addLight(Scene* s, Light light)
{
    obint id = addSceneObject(&light, s->lights, &s->lightCount, &s->lightCapacity, sizeof(light), &s->lightMap);
    LightHandle handle = {id};
    s->dirt |= ONYX_SCENE_LIGHTS_BIT;
    return handle;
}

static TextureHandle addTexture(Scene* s, Onyx_Texture texture)
{
    obint id = addSceneObject(&texture, s->textures, &s->textureCount, &s->textureCapacity, sizeof(texture), &s->texMap);
    TextureHandle handle = {id};
    addTextureToDirtyTextures(s, handle);
    TEXTURE(s, handle).dirt |= ONYX_TEX_ADDED_BIT;
    s->dirt |= ONYX_SCENE_TEXTURES_BIT;
    return handle;
}

static MaterialHandle addMaterial(Scene* s, Onyx_Material material)
{
    obint id = addSceneObject(&material, s->materials, &s->materialCount, &s->materialCapacity, sizeof(s->materials[0]), &s->matMap);
    MaterialHandle handle = {id};
    addMaterialToDirtyMaterials(s, handle);
    MATERIAL(s, handle).dirt |= ONYX_MAT_ADDED_BIT;
    s->dirt |= ONYX_SCENE_MATERIALS_BIT;
    return handle;
}

static void removePrim(Scene* s, Onyx_PrimitiveHandle handle)
{
    assert(handle.id < s->primCapacity);
    assert(handle.id > 0); //cant remove the default prim
    PRIM(s, handle).dirt |= ONYX_PRIM_REMOVED_BIT;
    addPrimToDirtyPrims(s, handle);
    s->dirt |= ONYX_SCENE_PRIMS_BIT;
}

static void removeLight(Scene* s, Onyx_LightHandle handle)
{
    assert(handle.id < s->lightCapacity);
    removeSceneObject(handle.id, s->lights, &s->lightCount, sizeof(s->lights[0]), &s->lightMap);
    s->dirt |= ONYX_SCENE_LIGHTS_BIT;
}

static void removeTexture(Scene* s, Onyx_TextureHandle handle)
{
    assert(handle.id < s->textureCapacity);
    assert(handle.id > 0); //cant remove the default prim
    TEXTURE(s, handle).dirt |= ONYX_TEX_REMOVED_BIT;
    addTextureToDirtyTextures(s, handle);
    s->dirt |= ONYX_SCENE_TEXTURES_BIT;
}

static void removeMaterial(Scene* s, Onyx_MaterialHandle handle)
{
    assert(handle.id < s->materialCapacity);
    removeSceneObject(handle.id, s->materials, &s->materialCount, sizeof(s->materials[0]), &s->matMap);
    s->dirt |= ONYX_SCENE_MATERIALS_BIT;
}

static LightHandle addDirectionLight(Scene* s, const Coal_Vec3 dir, const Coal_Vec3 color, const float intensity)
{
    Light light = {
        .type = ONYX_DIRECTION_LIGHT_TYPE,
        .intensity = intensity,
    };
    light.structure.directionLight.dir = dir;
    light.color = color;
    return addLight(s, light);
}

static LightHandle addPointLight(Scene* s, const Coal_Vec3 pos, const Coal_Vec3 color, const float intensity)
{
    Light light = {
        .type = ONYX_LIGHT_POINT_TYPE,
        .intensity = intensity
    };
    light.structure.pointLight.pos = pos;
    light.color = color;
    return addLight(s, light);
}

#define DEFAULT_TEX_DIM 4

static void createDefaultTexture(Scene* scene, Onyx_Memory* memory, Texture* texture)
{
    scene->defaultImage = onyx_CreateImageAndSampler(
        memory, DEFAULT_TEX_DIM, DEFAULT_TEX_DIM, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT, 1, VK_FILTER_LINEAR,
        ONYX_MEMORY_DEVICE_TYPE);

    Onyx_Command cmd = onyx_CreateCommand(onyx_GetMemoryInstance(memory), ONYX_V_QUEUE_GRAPHICS_TYPE);

    onyx_BeginCommandBufferOneTimeSubmit(cmd.buffer);

    Onyx_Barrier b = {};
    b.srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    b.dstStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    b.srcAccessMask = 0;
    b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    onyx_CmdTransitionImageLayout(cmd.buffer, b, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            scene->defaultImage.mipLevels, scene->defaultImage.handle);

    onyx_CmdClearColorImage(cmd.buffer, scene->defaultImage.handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1, 1.0, 1.0, 1.0, 1.0);

    b.srcStageFlags = b.dstStageFlags;
    b.dstStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    b.srcAccessMask = b.dstAccessMask;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    onyx_CmdTransitionImageLayout(cmd.buffer, b,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            scene->defaultImage.mipLevels, scene->defaultImage.handle);

    onyx_EndCommandBuffer(cmd.buffer);

    onyx_SubmitAndWait(&cmd, 0);

    onyx_DestroyCommand(cmd);

    texture->devImage = &scene->defaultImage;
}

static void printPrimInfoCmd(Hell_Grimoire* grim, void* scene)
{
    onyx_PrintPrimInfo(scene);
}

static void printTexInfoCmd(Hell_Grimoire* grim, void* scene)
{
    onyx_PrintTextureInfo(scene);
}

static Coal_Mat4
projectionMatrix(float fov, float aspect_ratio, float n, float f)
{
    float p = fov / aspect_ratio;
    float q = -fov;
    float A = f / (n - f);
    float B = f * n / (n - f);
    Mat4 m = {
        p,   0,  0,  0,
        0,   q,  0,  0,
        0,   0,  A,  -1,
        0,   0,  B,  0
    };
    return m;
}

void
onyx_CreateScene(Hell_Grimoire* grim, Onyx_Memory* memory, float fov,
                 float aspect_ratio, float nearClip, float farClip, Scene* scene)
{
    memset(scene, 0, sizeof(*scene));
    scene->memory = memory;
    scene->camera.xform = coal_Ident_Mat4();
    scene->camera.view = coal_Ident_Mat4();
    Mat4 m = coal_LookAt((Vec3){1, 1, 2}, (Vec3){0, 0, 0}, (Vec3){0, 1, 0});
    scene->camera.xform = m;
    scene->camera.view = coal_Invert4x4(m);
    scene->camera.proj = projectionMatrix(fov, aspect_ratio, nearClip, farClip);
    // set all xforms to identity
    scene->primCapacity = INIT_PRIM_CAP;
    scene->lightCapacity = INIT_LIGHT_CAP;
    scene->materialCapacity = INIT_MATERIAL_CAP;
    scene->textureCapacity = INIT_TEXTURE_CAP;
    createObjectMap(scene->primCapacity, 8, &scene->primMap);
    createObjectMap(scene->lightCapacity, 8, &scene->lightMap);
    createObjectMap(scene->materialCapacity, 8, &scene->matMap);
    createObjectMap(scene->textureCapacity, 8, &scene->texMap);

    scene->prims = hell_Malloc(scene->primCapacity * sizeof(scene->prims[0]));
    scene->lights = hell_Malloc(scene->lightCapacity * sizeof(scene->lights[0]));
    scene->materials = hell_Malloc(scene->materialCapacity * sizeof(scene->materials[0]));
    scene->textures = hell_Malloc(scene->textureCapacity * sizeof(scene->textures[0]));

    // 8 is arbitrary initial capacity
    hell_CreateArray(8, sizeof(Onyx_PrimitiveHandle), NULL, NULL, &scene->dirtyPrims);
    hell_CreateArray(8, sizeof(Onyx_MaterialHandle), NULL, NULL, &scene->dirtyMaterials);
    hell_CreateArray(8, sizeof(Onyx_TextureHandle), NULL, NULL, &scene->dirtyTextures);

    // create defaults
    Texture tex = {0};
    createDefaultTexture(scene, memory, &tex);
    TextureHandle texhandle = addTexture(scene, tex);
    MaterialHandle defaultMat = onyx_SceneCreateMaterial(scene, (Vec3){0, 0.937, 1.0}, 0.8, texhandle, NULL_TEXTURE, NULL_TEXTURE);
    Onyx_Primitive prim = {};
    scene->defaultGeo = onyx_CreateCube(memory, false);
    prim.geo = &scene->defaultGeo;
    prim.xform = COAL_MAT4_IDENT;
    prim.material = defaultMat;
    prim.flags = ONYX_PRIM_INVISIBLE_BIT;
    addPrim(scene, prim);

    scene->dirt = -1; // dirty everything

    if (grim)
    {
        hell_AddCommand(grim, "priminfo", printPrimInfoCmd, scene);
        hell_AddCommand(grim, "texinfo", printTexInfoCmd, scene);
    }
}

void onyx_BindPrimToMaterial(Scene* scene, Onyx_PrimitiveHandle primhandle, Onyx_MaterialHandle mathandle)
{
    assert(scene->materialCount > mathandle.id);
    PRIM(scene, primhandle).material = mathandle;

    scene->dirt |= ONYX_SCENE_PRIMS_BIT;
}

void onyx_BindPrimToMaterialDirect(Scene* scene, uint32_t directIndex, Onyx_MaterialHandle mathandle)
{
    assert(scene->materialCount > mathandle.id);
    scene->prims[directIndex].material = mathandle;

    scene->dirt |= ONYX_SCENE_PRIMS_BIT;
}

Onyx_PrimitiveHandle onyx_SceneAddPrim(Scene* scene, Onyx_Geometry* geo, const Coal_Mat4 xform, MaterialHandle mat)
{
    Onyx_Primitive prim = {
        .geo = geo,
        .xform = COAL_MAT4_IDENT,
        .material = mat
    };
    prim.xform = xform;
    PrimitiveHandle handle = addPrim(scene, prim);

    return handle;
}

Onyx_MaterialHandle onyx_SceneCreateMaterial(Onyx_Scene* scene, Vec3 color, float roughness,
        Onyx_TextureHandle albedoId, Onyx_TextureHandle roughnessId, Onyx_TextureHandle normalId)
{
    Onyx_Material mat = {0};

    mat.color = color;
    mat.roughness = roughness;
    mat.textureAlbedo    = albedoId;
    mat.textureRoughness = roughnessId;
    mat.textureNormal    = normalId;

    return addMaterial(scene, mat);
}

Onyx_LightHandle onyx_CreateDirectionLight(Scene* scene, const Vec3 color, const Vec3 direction)
{
    return addDirectionLight(scene, direction, color, 1.0);
}

Onyx_LightHandle onyx_SceneCreatePointLight(Scene* scene, const Vec3 color, const Vec3 position)
{
    return addPointLight(scene, position, color, 1.0);
}

#define HOME_POS    {0.0, 0.0, 1.0}
#define HOME_TARGET {0.0, 0.0, 0.0}
#define HOME_UP     {0.0, 1.0, 0.0}
#define ZOOM_RATE   0.005
#define PAN_RATE    0.1
#define TUMBLE_RATE 2

void onyx_UpdateCamera_LookAt(Scene* scene, Vec3 pos, Vec3 target, Vec3 up)
{
    scene->camera.xform = coal_LookAt(pos, target, up);
    scene->camera.view  = coal_Invert4x4(scene->camera.xform);
    scene->dirt |= ONYX_SCENE_CAMERA_VIEW_BIT;
}

static Vec3 prevpos;
static Vec3 prevtarget;
static Vec3 prevup;

void onyx_UpdateCamera_ArcBall(Scene* scene, Vec3* target, int screenWidth, int screenHeight, float dt, int xprev, int x, int yprev, int y, bool panning, bool tumbling, bool zooming, bool home)
{
    Vec3 pos = coal_GetTranslation_Mat4(scene->camera.xform);
    Vec3 up = coal_GetLocalY_Mat4(scene->camera.xform);
    int zoom_ticks = 0;
    if (zooming)
        zoom_ticks = x - xprev;
    else
        zoom_ticks = 0;
    if (home)
    {
        pos =    (Vec3)HOME_POS;
        *target = (Vec3)HOME_TARGET;
        up =     (Vec3)HOME_UP;
    }
    //pos = m_RotateY_Vec3(dt, &pos);
    arcball_camera_update(pos.e, target->e, up.e, NULL, dt,
            ZOOM_RATE, PAN_RATE, TUMBLE_RATE, screenWidth, screenHeight, xprev, x, yprev, y,
            panning, tumbling, zoom_ticks, 0);
    Mat4 m = coal_LookAt(pos, *target, up);
    scene->camera.xform = m;
    scene->camera.view  = coal_Invert4x4(scene->camera.xform);
    scene->dirt |= ONYX_SCENE_CAMERA_VIEW_BIT;
}

void onyx_SceneUpdateCamera_Pos(Onyx_Scene* scene, float dx, float dy, float dz)
{
    scene->camera.xform = coal_Translate_Mat4((Vec3){dx, dy, dz}, scene->camera.xform);
    scene->camera.view  = coal_Invert4x4(scene->camera.xform);
    scene->dirt |= ONYX_SCENE_CAMERA_VIEW_BIT;
}

void onyx_UpdateLight(Scene* scene, LightHandle handle, float intensity)
{
    LIGHT(scene, handle).intensity = intensity;
    scene->dirt |= ONYX_SCENE_LIGHTS_BIT;
}

void onyx_UpdatePrimXform(Scene* scene, PrimitiveHandle handle, const Mat4 delta)
{
    Coal_Mat4 M = coal_Mult_Mat4(PRIM(scene, handle).xform, delta);
    PRIM(scene, handle).xform = M;
    scene->dirt |= ONYX_SCENE_XFORMS_BIT;
}

void onyx_SetPrimXform(Onyx_Scene* scene, Onyx_PrimitiveHandle handle, Mat4 newXform)
{
    PRIM(scene, handle).xform = newXform;
    scene->dirt |= ONYX_SCENE_XFORMS_BIT;
}

Onyx_PrimitiveList onyx_CreatePrimList(uint32_t initial_cap)
{
    Onyx_PrimitiveList pl = {};
    hell_CreateArray(initial_cap, sizeof(Onyx_PrimitiveHandle), NULL, NULL, &pl.array);
    return pl;
}

void onyx_AddPrimToList(Onyx_PrimitiveHandle handle, Onyx_PrimitiveList* list)
{
    hell_ArrayPush(&list->array, &handle);
    // need to reset the pointer in case we realloc'd
}

void onyx_ClearPrimList(Onyx_PrimitiveList* list)
{
    hell_ArrayClear(&list->array);
}

void onyx_DestroyScene(Onyx_Scene* scene)
{
    memset(scene, 0, sizeof(*scene));
}

void onyx_SceneRemovePrim(Onyx_Scene* s, Onyx_PrimitiveHandle handle)
{
    removePrim(s, handle);
}

Onyx_LightHandle onyx_SceneAddDirectionLight(Scene* s, Coal_Vec3 dir, Coal_Vec3 color, float intensity)
{
    return addDirectionLight(s, dir, color, intensity);
}

Onyx_LightHandle onyx_SceneAddPointLight(Scene* s, Coal_Vec3 pos, Coal_Vec3 color, float intensity)
{
    return addPointLight(s, pos, color, intensity);
}

void onyx_SceneRemoveLight(Scene* s, LightHandle id)
{
    removeLight(s, id);
}

void onyx_PrintLightInfo(const Scene* s)
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
    for (int i = 0; i < s->lightCapacity; i++)
    {
        hell_Print(" %d:%d ", i, s->lightMap.indices[i]);
    }
    hell_Print("\n");
}

void onyx_PrintTextureInfo(const Scene* s)
{
    hell_Print("====== Scene: texture info =======\n");
    hell_Print("Texture count: %d\n", s->textureCount);
    for (int i = 0; i < s->textureCount; i++)
    {
        const Texture* tex = &s->textures[i];
        const Onyx_Image* img = tex->devImage;
        hell_Print("Texture index %d\n", i);
        hell_Print("Width %d Height %d Size %d \n", img->extent.width, img->extent.height, img->size);
        hell_Print("Format %d \n", img->format);
        hell_Print("\n");
    }
    hell_Print("Texture map: ");
    for (int i = 0; i < s->textureCapacity; i++)
    {
        hell_Print(" %d:%d ", i, s->texMap.indices[i]);
    }
    hell_Print("\n");
}

void onyx_PrintPrimInfo(const Scene* s)
{
    hell_Print("====== Scene: primitive info =======\n");
    hell_Print("Prim count: %d\n", s->primCount);
    for (int i = 0; i < s->primCount; i++)
    {
        hell_Print("Prim %d material id %d\n", i, s->prims[i].material.id);
        const Material* mat = &MATERIAL(s, s->prims[i].material);
        hell_Print("Material: handle id %d color %f %f %f roughness %f\n", s->prims[i].material.id, mat->color.r, mat->color.g, mat->color.b, mat->roughness);
        hell_Print("Material: Albedo TextureHandle: %d\n", mat->textureAlbedo.id);
        hell_Print_Mat4(s->prims[i].xform.e);
        hell_Print("\n");
    }
    hell_Print("Prim map: ");
    for (int i = 0; i < s->primCapacity; i++)
    {
        hell_Print(" %d:%d ", i, s->primMap.indices[i]);
    }
    hell_Print("\n");
}

void onyx_UpdateLightColor(Onyx_Scene* scene, Onyx_LightHandle handle, float r, float g, float b)
{
    LIGHT(scene, handle).color.r = r;
    LIGHT(scene, handle).color.g = g;
    LIGHT(scene, handle).color.b = b;
    scene->dirt |= ONYX_SCENE_LIGHTS_BIT;
}

void onyx_UpdateLightPos(Onyx_Scene* scene, Onyx_LightHandle handle, float x, float y, float z)
{
    LIGHT(scene, handle).structure.pointLight.pos.x = x;
    LIGHT(scene, handle).structure.pointLight.pos.y = y;
    LIGHT(scene, handle).structure.pointLight.pos.z = z;
    scene->dirt |= ONYX_SCENE_LIGHTS_BIT;
}

void onyx_UpdateLightIntensity(Onyx_Scene* scene, Onyx_LightHandle handle, float i)
{
    LIGHT(scene, handle).intensity = i;
    scene->dirt |= ONYX_SCENE_LIGHTS_BIT;
}

Onyx_Scene* onyx_AllocScene(void)
{
    return hell_Malloc(sizeof(Scene));
}

Mat4 onyx_SceneGetCameraView(const Onyx_Scene* scene)
{
    return scene->camera.view;
}

Mat4 onyx_SceneGetCameraProjection(const Onyx_Scene* scene)
{
    return scene->camera.proj;
}

Coal_Mat4 onyx_SceneGetCameraXform(const Onyx_Scene* scene)
{
    return scene->camera.xform;
}

Onyx_Primitive* onyx_GetPrimitive(const Onyx_Scene* s, uint32_t id)
{
    PrimitiveHandle handle = {id};
    return &PRIM(s, handle);
}

uint32_t onyx_SceneGetPrimCount(const Onyx_Scene* s)
{
    return s->primCount;
}

uint32_t onyx_SceneGetLightCount(const Onyx_Scene* s)
{
    return s->lightCount;
}

Onyx_Light* onyx_SceneGetLights(const Onyx_Scene* scene, uint32_t* count)
{
    *count = scene->lightCount;
    return scene->lights;
}

const Onyx_Light* Onyx_SceneGetLight(const Onyx_Scene* s, Onyx_LightHandle h)
{
    return &LIGHT(s, h);
}

Onyx_SceneDirtyFlags onyx_SceneGetDirt(const Onyx_Scene* s)
{
    return s->dirt;
}

void onyx_SceneEndFrame(Onyx_Scene* s)
{
    if (s->dirt & ONYX_SCENE_TEXTURES_BIT)
    {
        u32 c = s->dirtyTextures.count;
        TextureHandle* handles = s->dirtyTextures.elems;
        for (u32 i = 0; i < c; i++)
        {
            if (TEXTURE(s, handles[i]).dirt & ONYX_TEX_REMOVED_BIT)
            {
                u32 matcount = s->materialCount;
                for (u32 j = 0; j < matcount; j++)
                {
                    if (s->materials[j].textureAlbedo.id == handles[i].id)
                        s->materials[j].textureAlbedo = NULL_TEXTURE;
                    if (s->materials[j].textureRoughness.id == handles[i].id)
                        s->materials[j].textureRoughness = NULL_TEXTURE;
                    if (s->materials[j].textureNormal.id == handles[i].id)
                        s->materials[j].textureNormal = NULL_TEXTURE;
                }
                removeSceneObject(handles[i].id, s->textures, &s->textureCount,
                                  sizeof(s->textures[0]), &s->texMap);
            }
            else
            {
                TEXTURE(s, handles[i]).dirt = 0;
            }
        }
    }
    if (s->dirt & ONYX_SCENE_MATERIALS_BIT)
    {
        u32 c = s->dirtyMaterials.count;
        MaterialHandle* handles = s->dirtyMaterials.elems;
        for (u32 i = 0; i < c; i++)
        {
            Onyx_SceneObjectInt id = handles[i].id;
            if (MATERIAL(s, handles[i]).dirt & ONYX_MAT_REMOVED_BIT)
            {
                u32 primcount = s->primCount;
                for (u32 j = 0; j < primcount; j++)
                {
                    if (s->prims[j].material.id == id)
                        s->prims[j].material = NULL_MATERIAL;
                }
                removeSceneObject(id, s->materials, &s->materialCount,
                                  sizeof(s->materials[0]), &s->matMap);
            }
            else
            {
                MATERIAL(s, handles[i]).dirt = 0;
            }
        }
    }
    if (s->dirt & ONYX_SCENE_PRIMS_BIT)
    {
        u32 c = s->dirtyPrims.count;
        PrimitiveHandle* dp = s->dirtyPrims.elems;
        for (int i = 0; i < c; i++)
        {
            PrimitiveHandle handle = dp[i];
            // remove prims at the end of the frame so scene consumers can react to the prim
            // having the remove bit set
            if (PRIM(s, handle).dirt & ONYX_PRIM_REMOVED_BIT)
            {
                removeSceneObject(handle.id, s->prims, &s->primCount, sizeof(s->prims[0]), &s->primMap);
            }
            else
            {
                PRIM(s, dp[i]).dirt = 0;
            }
        }
    }
    s->dirt = 0;
    s->dirtyMaterials.count = 0;
    s->dirtyTextures.count = 0;
    s->dirtyPrims.count = 0;
}

Onyx_Texture* onyx_GetTexture(const Onyx_Scene* s, Onyx_TextureHandle handle)
{
    return &TEXTURE(s, handle);
}

Onyx_Material* onyx_GetMaterial(const Onyx_Scene* s, Onyx_MaterialHandle handle)
{
    return &MATERIAL(s, handle);
}

Onyx_TextureHandle onyx_SceneAddTexture(Onyx_Scene* scene, Onyx_Image* image)
{
    Onyx_Texture tex = {0};
    tex.devImage = image;
    return addTexture(scene, tex);
}

void
onyx_SceneRemoveTexture(Onyx_Scene* scene, Onyx_TextureHandle tex)
{
    removeTexture(scene, tex);
}

void
onyx_SceneRemoveMaterial(Onyx_Scene* scene, Onyx_MaterialHandle mat)
{
    removeMaterial(scene, mat);
}

uint32_t onyx_SceneGetTextureCount(const Onyx_Scene* s)
{
    return s->textureCount;
}

Onyx_Texture* onyx_SceneGetTextures(const Onyx_Scene* s, Onyx_SceneObjectInt* count)
{
    *count = s->textureCount;
    return s->textures;
}

uint32_t       onyx_SceneGetMaterialCount(const Onyx_Scene* s)
{
    return s->materialCount;
}

Onyx_Material* onyx_SceneGetMaterials(const Onyx_Scene* s, Onyx_SceneObjectInt* count)
{
    *count = s->materialCount;
    return s->materials;
}

uint32_t onyx_SceneGetMaterialIndex(const Onyx_Scene* s, Onyx_MaterialHandle handle)
{
    return s->matMap.indices[handle.id];
}

uint32_t onyx_SceneGetTextureIndex(const Onyx_Scene* s, Onyx_TextureHandle handle)
{
    return s->texMap.indices[handle.id];
}

void onyx_SceneDirtyTextures(Onyx_Scene* s)
{
    s->dirt |= ONYX_SCENE_TEXTURES_BIT;
}

void onyx_SceneSetGeoDirect(Onyx_Scene* s, Onyx_Geometry* geo, u32 directIndex)
{
    s->prims[directIndex].geo = geo;
    s->dirt |= ONYX_SCENE_PRIMS_BIT;
}

void onyx_SceneFreeGeoDirect(Onyx_Scene* s, u32 directIndex)
{
    onyx_FreeGeo(s->prims[directIndex].geo);
    memset(s->prims[directIndex].geo, 0, sizeof(*s->prims[directIndex].geo));;
    s->prims[directIndex].geo = NULL;
}

bool onyx_SceneHasGeoDirect(Onyx_Scene* s, u32 directIndex)
{
    if (s->prims[0].geo) return true;
    return false;
}

void onyx_SceneSetCameraXform(Onyx_Scene* scene, const Coal_Mat4 m)
{
    scene->camera.xform = m;
    scene->camera.view = coal_Invert4x4(m);
    scene->dirt |= ONYX_SCENE_CAMERA_VIEW_BIT;
}

void onyx_SceneSetCameraView(Onyx_Scene* scene, const Coal_Mat4 m)
{
    scene->camera.view = m;
    scene->camera.xform = coal_Invert4x4(m);
    scene->dirt |= ONYX_SCENE_CAMERA_VIEW_BIT;
}

const Onyx_Camera*
onyx_SceneGetCamera(const Onyx_Scene* scene)
{
    return &scene->camera;
}

void onyx_SceneSetCameraProjection(Onyx_Scene* scene, const Coal_Mat4 m)
{
    scene->camera.proj = m;
    scene->dirt |= ONYX_SCENE_CAMERA_PROJ_BIT;
}

Onyx_Geometry* onyx_SceneGetPrimGeo(Onyx_Scene* scene, PrimitiveHandle prim, Onyx_PrimDirtyFlags flags)
{
    addPrimToDirtyPrims(scene, prim);
    PRIM(scene, prim).dirt |= flags;
    scene->dirt |= ONYX_SCENE_PRIMS_BIT;
    return PRIM(scene, prim).geo;
}

Onyx_Primitive*
onyx_SceneGetPrimitive(Onyx_Scene* s, Onyx_PrimitiveHandle handle)
{
    return &PRIM(s, handle);
}

const Onyx_Primitive*
onyx_SceneGetPrimitiveConst(const Onyx_Scene* s, Onyx_PrimitiveHandle handle)
{
    return &PRIM(s, handle);
}

const Onyx_Primitive* onyx_SceneGetPrimitives(const Onyx_Scene* s, Onyx_SceneObjectInt* count)
{
    *count = s->primCount;
    return s->prims;
}

void onyx_SceneDirtyAll(Onyx_Scene* s)
{
    s->dirt = -1;
}

const Onyx_PrimitiveHandle* onyx_SceneGetDirtyPrimitives(const Onyx_Scene* s, uint32_t* count)
{
    *count = s->dirtyPrims.count;
    return s->dirtyPrims.elems;
}

void onyx_SceneCameraUpdateAspectRatio(Onyx_Scene* s, float ar)
{
    float fov = -s->camera.proj.e[1][1];
    float p   = fov / ar;
    s->camera.proj.e[0][0] = p;
    s->dirt |= ONYX_SCENE_CAMERA_PROJ_BIT;
}
