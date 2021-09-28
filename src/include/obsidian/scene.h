#ifndef OBDN_S_SCENE_H
#define OBDN_S_SCENE_H

#include "geo.h"
#include "types.h"
#include <coal/types.h>
#include <hell/cmd.h>

//#define OBDN_S_MAX_PRIMS     256
//#define OBDN_SCENE_MAX_PRIMS     32
//#define OBDN_SCENE_MAX_MATERIALS 16
//#define OBDN_SCENE_MAX_LIGHTS    8
//#define OBDN_SCENE_MAX_TEXTURES  16

typedef uint32_t  Obdn_SceneObjectInt;
typedef Coal_Mat4 Obdn_Xform;

#define OBDN_DEFHANDLE(name)                                                   \
    typedef struct Obdn_##name##Handle {                                       \
        Obdn_SceneObjectInt id;                                                \
    } Obdn_##name##Handle

OBDN_DEFHANDLE(Primitive);
OBDN_DEFHANDLE(Light);
OBDN_DEFHANDLE(Material);
OBDN_DEFHANDLE(Texture);

#define NULL_PRIM                                                              \
    (Obdn_PrimitiveHandle) { 0 }
#define NULL_LIGHT                                                             \
    (Obdn_LightHandle) { 0 }
#define NULL_MATERIAL                                                          \
    (Obdn_MaterialHandle) { 0 }
#define NULL_TEXTURE                                                           \
    (Obdn_TextureHandle) { 0 }

#define OBDN_S_NONE (uint32_t) - 1

typedef enum {
    OBDN_SCENE_CAMERA_VIEW_BIT = 1 << 0,
    OBDN_SCENE_CAMERA_PROJ_BIT = 1 << 1,
    OBDN_SCENE_LIGHTS_BIT      = 1 << 2,
    OBDN_SCENE_XFORMS_BIT      = 1 << 3,
    OBDN_SCENE_MATERIALS_BIT   = 1 << 4,
    OBDN_SCENE_TEXTURES_BIT    = 1 << 5,
    OBDN_SCENE_PRIMS_BIT       = 1 << 6,
} Obdn_SceneDirtyFlagBits;
typedef Obdn_Flags Obdn_SceneDirtyFlags;

typedef enum {
    OBDN_PRIM_ADDED_BIT            = 1 << 0,
    OBDN_PRIM_REMOVED_BIT          = 1 << 1,
    OBDN_PRIM_TOPOLOGY_CHANGED_BIT = 1 << 2,
} Obdn_PrimDirtyFlagBits;
typedef Obdn_Flags Obdn_PrimDirtyFlags;

typedef enum {
    OBDN_PRIM_INVISIBLE_BIT           = 1 << 0,
} Obdn_PrimFlagBits;
typedef Obdn_Flags Obdn_PrimFlags;

typedef struct {
    Coal_Mat4 xform;
    Coal_Mat4 view; // xform inverted
    Coal_Mat4 proj;
} Obdn_Camera;

typedef enum {
    OBDN_LIGHT_POINT_TYPE,
    OBDN_DIRECTION_LIGHT_TYPE
} Obdn_LightType;

typedef struct {
    Coal_Vec3 pos;
} Obdn_PointLight;

typedef struct {
    Coal_Vec3 dir;
} Obdn_DirectionLight;

typedef struct {
    Obdn_Geometry*      geo;
    Obdn_Xform          xform;
    Obdn_MaterialHandle material;
    Obdn_PrimDirtyFlags dirt;
    Obdn_PrimFlags      flags;
} Obdn_Primitive;

typedef union {
    Obdn_PointLight     pointLight;
    Obdn_DirectionLight directionLight;
} Obdn_LightStructure;

typedef struct {
    Obdn_LightStructure structure;
    float               intensity;
    Coal_Vec3           color;
    Obdn_LightType      type;
} Obdn_Light;

typedef struct {
    Obdn_Image*       devImage;
} Obdn_Texture;

typedef struct {
    Coal_Vec3          color;
    float              roughness;
    Obdn_TextureHandle textureAlbedo;
    Obdn_TextureHandle textureRoughness;
    Obdn_TextureHandle textureNormal;
    Obdn_TextureHandle padding;
} Obdn_Material;

#if UNIX
_Static_assert(
    sizeof(Obdn_Material) == 32,
    "Changing material size and layout may conflict with shader uniforms");
#endif

typedef struct Obdn_Scene Obdn_Scene;

// container to associate prim ids with pipelines
typedef struct {
    Obdn_SceneObjectInt  primCount;
    Obdn_SceneObjectInt* primIds;
} Obdn_PrimitiveList;

// New paradigm: scene does not own any gpu resources. Images, geo, anything backed by gpu memory
// Those things should be created and destroyed independently
// This allows us to avoid confusion when multiple entities are sharing a scene and hopefully 
// avoid double-freeing reources.
Obdn_Scene* obdn_AllocScene(void);
void obdn_CreateScene(Hell_Grimoire* grim, Obdn_Memory* memory, float fov,
                 float aspectRatio, float nearDepth, float farClip, Obdn_Scene* scene);

// will update camera as well as target.
void obdn_UpdateCamera_ArcBall(Obdn_Scene* scene, Coal_Vec3* target,
                               int screenWidth, int screenHeight, float dt,
                               int xprev, int x, int yprev, int y, bool panning,
                               bool tumbling, bool zooming, bool home);
void obdn_UpdateCamera_LookAt(Obdn_Scene* scene, Coal_Vec3 pos,
                              Coal_Vec3 target, Coal_Vec3 up);
void obdn_SceneUpdateCamera_Pos(Obdn_Scene* scene, float dx, float dy, float dz);
void obdn_CreateEmptyScene(Obdn_Scene* scene);
void obdn_UpdateLight(Obdn_Scene* scene, Obdn_LightHandle handle,
                      float intensity);
void obdn_BindPrimToMaterial(Obdn_Scene*                scene,
                             const Obdn_PrimitiveHandle Obdn_PrimitiveHandle,
                             const Obdn_MaterialHandle  matId);
// this does not go through the prim map. allows us to preemptively bind prims
// to materials before we have an actual handle to them. practically useful for
// texture painting to always bind a material to the first prim
void obdn_BindPrimToMaterialDirect(Obdn_Scene* scene, uint32_t directIndex,
                                   Obdn_MaterialHandle mathandle);

void obdn_AddPrimToList(const Obdn_PrimitiveHandle, Obdn_PrimitiveList*);
void obdn_AddDirectionLight(Obdn_Scene* s, Coal_Vec3 dir, Coal_Vec3 color,
                            float intensity);
void obdn_ClearPrimList(Obdn_PrimitiveList*);
void obdn_DestroyScene(Obdn_Scene* scene);
void obdn_AddPointLight(Obdn_Scene* s, Coal_Vec3 pos, Coal_Vec3 color,
                        float intensity);
void obdn_PrintLightInfo(const Obdn_Scene* s);
void obdn_PrintTextureInfo(const Obdn_Scene* s);
void obdn_PrintPrimInfo(const Obdn_Scene* s);
void obdn_SceneRemoveLight(Obdn_Scene* s, Obdn_LightHandle id);

// returns an array of prim handle and stores the count in count
const Obdn_PrimitiveHandle* obdn_SceneGetDirtyPrimitives(const Obdn_Scene*,
                                                         uint32_t* count);

void obdn_SceneRemoveTexture(Obdn_Scene* scene, Obdn_TextureHandle tex);

// Does not free geo
void obdn_SceneRemovePrim(Obdn_Scene* s, Obdn_PrimitiveHandle id);

// Does not own the geo. Will not free if prim is removed.
Obdn_PrimitiveHandle obdn_SceneAddPrim(Obdn_Scene* scene, Obdn_Geometry* geo,
                                  const Coal_Mat4     xform,
                                  Obdn_MaterialHandle mat);

Obdn_LightHandle     obdn_CreateDirectionLight(Obdn_Scene*     scene,
                                               const Coal_Vec3 color,
                                               const Coal_Vec3 direction);
Obdn_LightHandle obdn_CreatePointLight(Obdn_Scene* scene, const Coal_Vec3 color,
                                       const Coal_Vec3 position);
// a texture id of 0 means no texture will be used
Obdn_MaterialHandle obdn_SceneCreateMaterial(Obdn_Scene* scene, Coal_Vec3 color,
                                             float              roughness,
                                             Obdn_TextureHandle albedoId,
                                             Obdn_TextureHandle roughnessId,
                                             Obdn_TextureHandle normalId);

// swaps an rprim into a scene. returns the rprim that was there.
Obdn_Geometry obdn_SwapRPrim(Obdn_Scene* scene, const Obdn_Geometry* newRprim,
                             const Obdn_PrimitiveHandle id);

void obdn_UpdateLightColor(Obdn_Scene* scene, Obdn_LightHandle id, float r,
                           float g, float b);
void obdn_UpdateLightIntensity(Obdn_Scene* scene, Obdn_LightHandle id, float i);
void obdn_UpdateLightPos(Obdn_Scene* scene, Obdn_LightHandle id, float x,
                         float y, float z);
void obdn_UpdatePrimXform(Obdn_Scene* scene, const Obdn_PrimitiveHandle primId,
                          const Coal_Mat4 delta);
void obdn_SetPrimXform(Obdn_Scene* scene, Obdn_PrimitiveHandle primId,
                       Coal_Mat4 newXform);

Coal_Mat4 obdn_GetCameraView(const Obdn_Scene* scene);
Coal_Mat4 obdn_GetCameraProjection(const Obdn_Scene* scene);

void obdn_SceneSetCameraView(Obdn_Scene* scene, const Coal_Mat4);
void obdn_SceneSetCameraProjection(Obdn_Scene* scene, const Coal_Mat4);

Obdn_Primitive* obdn_GetPrimitive(const Obdn_Scene* s, uint32_t id);

uint32_t obdn_GetPrimCount(const Obdn_Scene*);

Obdn_SceneDirtyFlags obdn_GetSceneDirt(const Obdn_Scene*);

// sets dirt flags to 0 and resets dirty prim set
void obdn_SceneEndFrame(Obdn_Scene*);

// Does not own the image
Obdn_TextureHandle obdn_SceneAddTexture(Obdn_Scene*  scene,
                                           Obdn_Image* image);

Obdn_Material* obdn_GetMaterial(const Obdn_Scene*   s,
                                Obdn_MaterialHandle handle);

// given a handle returns the index into the raw resource array
uint32_t obdn_SceneGetMaterialIndex(const Obdn_Scene*, Obdn_MaterialHandle);

// given a handle returns the index into the raw resource array
uint32_t obdn_SceneGetTextureIndex(const Obdn_Scene*, Obdn_TextureHandle);

uint32_t       obdn_SceneGetTextureCount(const Obdn_Scene* s);
Obdn_Texture*  obdn_SceneGetTextures(const Obdn_Scene* s);
uint32_t       obdn_SceneGetMaterialCount(const Obdn_Scene* s);
Obdn_Material* obdn_SceneGetMaterials(const Obdn_Scene* s);

// a bit of a hack for dali
void obdn_SceneDirtyTextures(Obdn_Scene* s);

void obdn_SceneSetGeoDirect(Obdn_Scene* s, Obdn_Geometry* geo,
                            uint32_t directIndex);

void obdn_SceneFreeGeoDirect(Obdn_Scene* s, uint32_t directIndex);

bool obdn_SceneHasGeoDirect(Obdn_Scene* s, uint32_t directIndex);

const Obdn_Camera* obdn_SceneGetCamera(const Obdn_Scene* scene);

// Writeable access to the geo held by the prim. 
// Must pass flags to indicate how the geo will be modified.
// Can return NULL indicating prim has no geo yet.
Obdn_Geometry* obdn_SceneGetPrimGeo(Obdn_Scene*          scene,
                                    Obdn_PrimitiveHandle prim,
                                    Obdn_PrimDirtyFlags  flags);

Obdn_Primitive* 
obdn_SceneGetPrimitive(Obdn_Scene* s, Obdn_PrimitiveHandle handle);

void obdn_SceneDirtyAll(Obdn_Scene* s);

void obdn_SceneCameraUpdateAspectRatio(Obdn_Scene* s, float ar);

#endif /* end of include guard: OBDN_S_SCENE_H */
