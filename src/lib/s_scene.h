#ifndef OBDN_S_SCENE_H
#define OBDN_S_SCENE_H

#include "r_geo.h"
#include "types.h"
#include <coal/coal.h>

//#define OBDN_S_MAX_PRIMS     256
#define OBDN_SCENE_MAX_PRIMS     32
#define OBDN_SCENE_MAX_MATERIALS 16
#define OBDN_SCENE_MAX_LIGHTS    8
#define OBDN_SCENE_MAX_TEXTURES  16

typedef uint32_t  Obdn_PrimId;
typedef uint32_t  Obdn_LightId;
typedef uint32_t  Obdn_MaterialId;
typedef uint32_t  Obdn_TextureId;
typedef Coal_Mat4 Obdn_Xform;

#define OBDN_S_NONE (uint32_t)-1

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

typedef struct {
    Mat4 xform;
    Mat4 view; //xform inverted
    Mat4 proj;
} Obdn_Camera;

typedef enum {
    OBDN_LIGHT_POINT_TYPE,
    OBDN_DIRECTION_LIGHT_TYPE
} Obdn_LightType;

typedef struct {
    Vec3 pos;
} Obdn_PointLight;

typedef struct {
    Vec3 dir;
} Obdn_DirectionLight;

typedef struct {
    Obdn_Geometry   geo;
    Obdn_Xform      xform;
    Obdn_MaterialId materialId;
} Obdn_Primitive;

typedef union {
    Obdn_PointLight     pointLight;
    Obdn_DirectionLight directionLight;
} Obdn_LightStructure;

typedef struct {
    Obdn_LightStructure  structure;
    float                  intensity;
    Vec3                   color;
    Obdn_LightType       type;
} Obdn_Light;

typedef struct {
    Obdn_V_Image        devImage;
    Obdn_V_BufferRegion hostBuffer;
} Obdn_Texture;

typedef struct {
    Vec3              color;
    float             roughness;
    Obdn_TextureId textureAlbedo;
    Obdn_TextureId textureRoughness;
    Obdn_TextureId textureNormal;
    Obdn_TextureId padding;
} Obdn_Material;

typedef struct Obdn_Scene {
    Obdn_PrimId     primCount;
    Obdn_LightId    lightCount;
    Obdn_MaterialId materialCount;
    Obdn_TextureId  textureCount;
    Obdn_Primitive  prims[OBDN_SCENE_MAX_PRIMS];
    Obdn_Material   materials[OBDN_SCENE_MAX_MATERIALS];
    Obdn_Texture    textures[OBDN_SCENE_MAX_TEXTURES];
    Obdn_Light      lights[OBDN_SCENE_MAX_LIGHTS];
    Obdn_Camera     camera;
    Obdn_SceneDirtyFlags dirt;
} Obdn_Scene;

// container to associate prim ids with pipelines
typedef struct {
    Obdn_PrimId primCount;
    Obdn_PrimId primIds[OBDN_SCENE_MAX_PRIMS];
} Obdn_PrimitiveList;

Obdn_Scene* obdn_AllocScene(void);

void obdn_CreateScene(uint16_t windowWidth, uint16_t windowHeight, float nearClip, float farClip, Obdn_Scene* scene);

// will update camera as well as target.
void obdn_UpdateCamera_ArcBall(Obdn_Scene* scene, Vec3* target, int screenWidth, int screenHeight, float dt, int xprev, int x, int yprev, int y, bool panning, bool tumbling, bool zooming, bool home);
void obdn_UpdateCamera_LookAt(Obdn_Scene* scene, Vec3 pos, Vec3 target, Vec3 up);
void obdn_CreateEmptyScene(Obdn_Scene* scene);
void obdn_UpdateLight(Obdn_Scene* scene, uint32_t id, float intensity);
void obdn_BindPrimToMaterial(Obdn_Scene* scene, const Obdn_PrimId primId, const Obdn_MaterialId matId);

void  obdn_AddPrimToList(const Obdn_PrimId, Obdn_PrimitiveList*);
void obdn_AddDirectionLight(Obdn_Scene* s, Coal_Vec3 dir, Coal_Vec3 color, float intensity);
void  obdn_ClearPrimList(Obdn_PrimitiveList*);
void  obdn_CleanUpScene(Obdn_Scene* scene);
void obdn_AddPointLight(Obdn_Scene* s, Coal_Vec3 pos, Coal_Vec3 color, float intensity);
void obdn_PrintLightInfo(const Obdn_Scene* s);
void obdn_PrintPrimInfo(const Obdn_Scene* s);
void obdn_RemoveLight(Obdn_Scene* s, Obdn_LightId id);

bool obdn_PrimExists(const Obdn_Scene* s, Obdn_PrimId id);

Obdn_PrimId obdn_LoadPrim(Obdn_Scene* scene, Obdn_Memory* memory, const char* filePath, const Coal_Mat4 xform);
void             obdn_RemovePrim(Obdn_Scene* s, Obdn_PrimId id);
Obdn_PrimId    obdn_AddPrim(Obdn_Scene* scene, const Obdn_Geometry prim, const Coal_Mat4 xform);
Obdn_TextureId obdn_LoadTexture(Obdn_Scene* scene, Obdn_Memory*, const char* filePath, const uint8_t channelCount);
Obdn_LightId   obdn_CreateDirectionLight(Obdn_Scene* scene, const Vec3 color, const Vec3 direction);
Obdn_LightId   obdn_CreatePointLight(Obdn_Scene* scene, const Vec3 color, const Vec3 position);
// a texture id of 0 means no texture will be used
Obdn_MaterialId obdn_CreateMaterial(Obdn_Scene* scene, Vec3 color, float roughness, 
        Obdn_TextureId albedoId, 
        Obdn_TextureId roughnessId,
        Obdn_TextureId normalId);

// swaps an rprim into a scene. returns the rprim that was there. 
Obdn_Geometry obdn_SwapRPrim(Obdn_Scene* scene, const Obdn_Geometry* newRprim, const Obdn_PrimId id);

void obdn_UpdateLightColor(Obdn_Scene* scene, Obdn_LightId id, float r, float g, float b);
void obdn_UpdateLightIntensity(Obdn_Scene* scene, Obdn_LightId id, float i);
void obdn_UpdateLightPos(Obdn_Scene* scene, Obdn_LightId id, float x, float y, float z);
void obdn_UpdatePrimXform(Obdn_Scene* scene, const Obdn_PrimId primId, const Mat4 delta);

#endif /* end of include guard: OBDN_S_SCENE_H */
