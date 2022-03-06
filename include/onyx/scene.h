#ifndef ONYX_S_SCENE_H
#define ONYX_S_SCENE_H

#include "geo.h"
#include "types.h"
#include <coal/types.h>
#include <hell/cmd.h>
#include <hell/ds.h>

//#define ONYX_S_MAX_PRIMS     256
//#define ONYX_SCENE_MAX_PRIMS     32
//#define ONYX_SCENE_MAX_MATERIALS 16
//#define ONYX_SCENE_MAX_LIGHTS    8
//#define ONYX_SCENE_MAX_TEXTURES  16

typedef uint32_t  Onyx_SceneObjectInt;
#ifdef ONYX_SIMPLE_TYPE_NAMES
typedef Onyx_SceneObjectInt obint;
#endif
typedef Coal_Mat4 Onyx_Xform;

#define ONYX_DEFHANDLE(name)                                                   \
    typedef struct Onyx_##name##Handle {                                       \
        Onyx_SceneObjectInt id;                                                \
    } Onyx_##name##Handle

ONYX_DEFHANDLE(Primitive);
ONYX_DEFHANDLE(Light);
ONYX_DEFHANDLE(Material);
ONYX_DEFHANDLE(Texture);

#define NULL_PRIM                                                              \
    (Onyx_PrimitiveHandle) { 0 }
#define NULL_LIGHT                                                             \
    (Onyx_LightHandle) { 0 }
#define NULL_MATERIAL                                                          \
    (Onyx_MaterialHandle) { 0 }
#define NULL_TEXTURE                                                           \
    (Onyx_TextureHandle) { 0 }

#define ONYX_S_NONE (uint32_t) - 1

typedef enum {
    ONYX_SCENE_CAMERA_VIEW_BIT = 1 << 0,
    ONYX_SCENE_CAMERA_PROJ_BIT = 1 << 1,
    ONYX_SCENE_LIGHTS_BIT      = 1 << 2,
    ONYX_SCENE_XFORMS_BIT      = 1 << 3,
    ONYX_SCENE_MATERIALS_BIT   = 1 << 4,
    ONYX_SCENE_TEXTURES_BIT    = 1 << 5,
    ONYX_SCENE_PRIMS_BIT       = 1 << 6,
    ONYX_SCENE_LAST_BIT_PLUS_1,
} Onyx_SceneDirtyFlagBits;
typedef uint8_t Onyx_SceneDirtyFlags;

typedef enum {
    ONYX_PRIM_ADDED_BIT            = 1 << 0,
    ONYX_PRIM_REMOVED_BIT          = 1 << 1,
    ONYX_PRIM_TOPOLOGY_CHANGED_BIT = 1 << 2,
    ONYX_PRIM_LAST_BIT_PLUS_1,
} Onyx_PrimDirtyFlagBits;
typedef uint8_t Onyx_PrimDirtyFlags;

typedef enum {
    ONYX_MAT_ADDED_BIT   = 1 << 0,
    ONYX_MAT_REMOVED_BIT = 1 << 1,
    ONYX_MAT_CHANGED_BIT = 1 << 2,
    ONYX_MAT_LAST_BIT_PLUS_1,
} Onyx_MatDirtyFlagBits;
typedef uint8_t Onyx_MatDirtyFlags;

typedef enum {
    ONYX_TEX_ADDED_BIT   = 1 << 0,
    ONYX_TEX_REMOVED_BIT = 1 << 1,
    ONYX_TEX_CHANGED_BIT = 1 << 2,
    ONYX_TEX_LAST_BIT_PLUS_1,
} Onyx_TexDirtyFlagBits;
typedef uint8_t Onyx_TexDirtyFlags;

#ifdef __cplusplus
#define STATIC_ASSERT static_assert
#else
#define STATIC_ASSERT _Static_assert
#endif
STATIC_ASSERT(ONYX_TEX_LAST_BIT_PLUS_1 < 255, "Tex dirty bits must fit in a byte.");
STATIC_ASSERT(ONYX_MAT_LAST_BIT_PLUS_1 < 255, "Mat dirty bits must fit in a byte.");
STATIC_ASSERT(ONYX_PRIM_LAST_BIT_PLUS_1 < 255, "Prim dirty bits must fit in a byte.");
STATIC_ASSERT(ONYX_SCENE_LAST_BIT_PLUS_1 < 255, "Scene dirty bits must fit in a byte.");

typedef enum {
    ONYX_PRIM_INVISIBLE_BIT           = 1 << 0,
} Onyx_PrimFlagBits;
typedef Onyx_Flags Onyx_PrimFlags;

typedef struct {
    Coal_Mat4 xform;
    Coal_Mat4 view; // xform inverted
    Coal_Mat4 proj;
} Onyx_Camera;

typedef enum {
    ONYX_LIGHT_POINT_TYPE,
    ONYX_DIRECTION_LIGHT_TYPE
} Onyx_LightType;

typedef struct {
    Coal_Vec3 pos;
} Onyx_PointLight;

typedef struct {
    Coal_Vec3 dir;
} Onyx_DirectionLight;

typedef struct {
    Onyx_Geometry*      geo;
    Onyx_Xform          xform;
    Onyx_MaterialHandle material;
    Onyx_PrimDirtyFlags dirt;
    Onyx_PrimFlags      flags;
} Onyx_Primitive;

typedef union {
    Onyx_PointLight     pointLight;
    Onyx_DirectionLight directionLight;
} Onyx_LightStructure;

typedef struct {
    Onyx_LightStructure structure;
    float               intensity;
    Coal_Vec3           color;
    Onyx_LightType      type;
} Onyx_Light;

typedef struct {
    Onyx_Image*        devImage;
    Onyx_TexDirtyFlags dirt;
} Onyx_Texture;

typedef struct {
    Coal_Vec3             color;
    float                 roughness;
    Onyx_TextureHandle    textureAlbedo;
    Onyx_TextureHandle    textureRoughness;
    Onyx_TextureHandle    textureNormal;
    Onyx_MatDirtyFlagBits dirt;
} Onyx_Material;

typedef struct Onyx_Scene Onyx_Scene;

// container to associate prim ids with pipelines
typedef struct {
    Hell_Array array; // backing array. should not need to access directly
} Onyx_PrimitiveList;

static inline const Onyx_PrimitiveHandle* onyx_GetPrimlistPrims(const Onyx_PrimitiveList* list, u32* count)
{
    *count = list->array.count;
    return (Onyx_PrimitiveHandle*)list->array.elems;
}

// New paradigm: scene does not own any gpu resources. Images, geo, anything backed by gpu memory
// Those things should be created and destroyed independently
// This allows us to avoid confusion when multiple entities are sharing a scene and hopefully 
// avoid double-freeing reources.
Onyx_Scene* onyx_AllocScene(void);
void onyx_CreateScene(Hell_Grimoire* grim, Onyx_Memory* memory, float fov,
                 float aspectRatio, float nearDepth, float farClip, Onyx_Scene* scene);

// will update camera as well as target.
void onyx_UpdateCamera_ArcBall(Onyx_Scene* scene, Coal_Vec3* target,
                               int screenWidth, int screenHeight, float dt,
                               int xprev, int x, int yprev, int y, bool panning,
                               bool tumbling, bool zooming, bool home);
void onyx_UpdateCamera_LookAt(Onyx_Scene* scene, Coal_Vec3 pos,
                              Coal_Vec3 target, Coal_Vec3 up);
void onyx_SceneUpdateCamera_Pos(Onyx_Scene* scene, float dx, float dy, float dz);
void onyx_CreateEmptyScene(Onyx_Scene* scene);
void onyx_UpdateLight(Onyx_Scene* scene, Onyx_LightHandle handle,
                      float intensity);
void onyx_BindPrimToMaterial(Onyx_Scene*                scene,
                             const Onyx_PrimitiveHandle Onyx_PrimitiveHandle,
                             const Onyx_MaterialHandle  matId);
// this does not go through the prim map. allows us to preemptively bind prims
// to materials before we have an actual handle to them. practically useful for
// texture painting to always bind a material to the first prim
void onyx_BindPrimToMaterialDirect(Onyx_Scene* scene, uint32_t directIndex,
                                   Onyx_MaterialHandle mathandle);

Onyx_PrimitiveList onyx_CreatePrimList(uint32_t initial_capacity);
void onyx_AddPrimToList(const Onyx_PrimitiveHandle, Onyx_PrimitiveList*);
Onyx_LightHandle onyx_SceneAddDirectionLight(Onyx_Scene* s, Coal_Vec3 dir, Coal_Vec3 color,
                            float intensity);
void onyx_ClearPrimList(Onyx_PrimitiveList*);
void onyx_DestroyScene(Onyx_Scene* scene);
Onyx_LightHandle onyx_SceneAddPointLight(Onyx_Scene* s, Coal_Vec3 pos, Coal_Vec3 color,
                        float intensity);
void onyx_PrintLightInfo(const Onyx_Scene* s);
void onyx_PrintTextureInfo(const Onyx_Scene* s);
void onyx_PrintPrimInfo(const Onyx_Scene* s);
void onyx_SceneRemoveLight(Onyx_Scene* s, Onyx_LightHandle id);

// returns an array of prim handle and stores the count in count
const Onyx_PrimitiveHandle* onyx_SceneGetDirtyPrimitives(const Onyx_Scene*,
                                                         uint32_t* count);

void onyx_SceneRemoveTexture(Onyx_Scene* scene, Onyx_TextureHandle tex);

void onyx_SceneRemoveMaterial(Onyx_Scene* scene, Onyx_MaterialHandle mat);

// Does not free geo
void onyx_SceneRemovePrim(Onyx_Scene* s, Onyx_PrimitiveHandle id);

// Does not own the geo. Will not free if prim is removed.
Onyx_PrimitiveHandle onyx_SceneAddPrim(Onyx_Scene* scene, Onyx_Geometry* geo,
                                  const Coal_Mat4     xform,
                                  Onyx_MaterialHandle mat);

Onyx_LightHandle     onyx_CreateDirectionLight(Onyx_Scene*     scene,
                                               const Coal_Vec3 color,
                                               const Coal_Vec3 direction);
Onyx_LightHandle onyx_CreatePointLight(Onyx_Scene* scene, const Coal_Vec3 color,
                                       const Coal_Vec3 position);
// a texture id of 0 means no texture will be used
Onyx_MaterialHandle onyx_SceneCreateMaterial(Onyx_Scene* scene, Coal_Vec3 color,
                                             float              roughness,
                                             Onyx_TextureHandle albedoId,
                                             Onyx_TextureHandle roughnessId,
                                             Onyx_TextureHandle normalId);

// swaps an rprim into a scene. returns the rprim that was there.
Onyx_Geometry onyx_SwapRPrim(Onyx_Scene* scene, const Onyx_Geometry* newRprim,
                             const Onyx_PrimitiveHandle id);

void onyx_UpdateLightColor(Onyx_Scene* scene, Onyx_LightHandle id, float r,
                           float g, float b);
void onyx_UpdateLightIntensity(Onyx_Scene* scene, Onyx_LightHandle id, float i);
void onyx_UpdateLightPos(Onyx_Scene* scene, Onyx_LightHandle id, float x,
                         float y, float z);
void onyx_UpdatePrimXform(Onyx_Scene* scene, const Onyx_PrimitiveHandle primId,
                          const Coal_Mat4 delta);
void onyx_SetPrimXform(Onyx_Scene* scene, Onyx_PrimitiveHandle primId,
                       Coal_Mat4 newXform);

Coal_Mat4 onyx_SceneGetCameraView(const Onyx_Scene* scene);
Coal_Mat4 onyx_SceneGetCameraProjection(const Onyx_Scene* scene);
Coal_Mat4 onyx_SceneGetCameraXform(const Onyx_Scene* scene);

void onyx_SceneSetCameraXform(Onyx_Scene* scene, const Coal_Mat4);
void onyx_SceneSetCameraView(Onyx_Scene* scene, const Coal_Mat4);
void onyx_SceneSetCameraProjection(Onyx_Scene* scene, const Coal_Mat4);

Onyx_Primitive* onyx_GetPrimitive(const Onyx_Scene* s, uint32_t id);

uint32_t onyx_SceneGetPrimCount(const Onyx_Scene*);
uint32_t onyx_SceneGetLightCount(const Onyx_Scene*);

Onyx_Light* onyx_SceneGetLights(const Onyx_Scene*, uint32_t* count);

const Onyx_Light* Onyx_SceneGetLight(const Onyx_Scene*, Onyx_LightHandle);

Onyx_SceneDirtyFlags onyx_SceneGetDirt(const Onyx_Scene*);

// sets dirt flags to 0 and resets dirty prim set
void onyx_SceneEndFrame(Onyx_Scene*);

// Does not own the image
Onyx_TextureHandle onyx_SceneAddTexture(Onyx_Scene*  scene,
                                           Onyx_Image* image);

Onyx_Material* onyx_GetMaterial(const Onyx_Scene*   s,
                                Onyx_MaterialHandle handle);

// given a handle returns the index into the raw resource array
uint32_t onyx_SceneGetMaterialIndex(const Onyx_Scene*, Onyx_MaterialHandle);

// given a handle returns the index into the raw resource array
uint32_t onyx_SceneGetTextureIndex(const Onyx_Scene*, Onyx_TextureHandle);

uint32_t       onyx_SceneGetTextureCount(const Onyx_Scene* s);
Onyx_Texture*  onyx_SceneGetTextures(const Onyx_Scene* s, Onyx_SceneObjectInt* count);
uint32_t       onyx_SceneGetMaterialCount(const Onyx_Scene* s);
Onyx_Material* onyx_SceneGetMaterials(const Onyx_Scene* s, Onyx_SceneObjectInt* count);

// a bit of a hack for dali
void onyx_SceneDirtyTextures(Onyx_Scene* s);

void onyx_SceneSetGeoDirect(Onyx_Scene* s, Onyx_Geometry* geo,
                            uint32_t directIndex);

void onyx_SceneFreeGeoDirect(Onyx_Scene* s, uint32_t directIndex);

bool onyx_SceneHasGeoDirect(Onyx_Scene* s, uint32_t directIndex);

const Onyx_Camera* onyx_SceneGetCamera(const Onyx_Scene* scene);

// Writeable access to the geo held by the prim. 
// Must pass flags to indicate how the geo will be modified.
// Can return NULL indicating prim has no geo yet.
Onyx_Geometry* onyx_SceneGetPrimGeo(Onyx_Scene*          scene,
                                    Onyx_PrimitiveHandle prim,
                                    Onyx_PrimDirtyFlags  flags);

Onyx_Primitive* 
onyx_SceneGetPrimitive(Onyx_Scene* s, Onyx_PrimitiveHandle handle);

const Onyx_Primitive*
onyx_SceneGetPrimitiveConst(const Onyx_Scene* s, Onyx_PrimitiveHandle handle);

const Onyx_Primitive* onyx_SceneGetPrimitives(const Onyx_Scene* s, Onyx_SceneObjectInt* count);

void onyx_SceneDirtyAll(Onyx_Scene* s);

void onyx_SceneCameraUpdateAspectRatio(Onyx_Scene* s, float ar);

static inline Onyx_PrimitiveHandle onyx_CreatePrimitiveHandle(Onyx_SceneObjectInt i) 
{
    return (Onyx_PrimitiveHandle){.id = i}; 
}

static inline Onyx_MaterialHandle onyx_CreateMaterialHandle(Onyx_SceneObjectInt i) 
{
    return (Onyx_MaterialHandle){.id = i}; 
}

static inline Onyx_TextureHandle onyx_CreateTextureHandle(Onyx_SceneObjectInt i) 
{
    return (Onyx_TextureHandle){.id = i}; 
}

static inline Onyx_LightHandle onyx_CreateLightHandle(Onyx_SceneObjectInt i) 
{
    return (Onyx_LightHandle){.id = i}; 
}

#endif /* end of include guard: ONYX_S_SCENE_H */
