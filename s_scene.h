#ifndef OBDN_S_SCENE_H
#define OBDN_S_SCENE_H

#include "r_geo.h"
#include "t_def.h"

#define OBDN_S_MAX_PRIMS     256 
#define OBDN_S_MAX_MATERIALS 16
#define OBDN_S_MAX_LIGHTS    16
#define OBDN_S_MAX_TEXTURES  16

typedef Obdn_Mask Obdn_S_DirtyMask;
typedef uint32_t   Obdn_S_PrimId;
typedef uint32_t   Obdn_S_LightId;
typedef uint32_t   Obdn_S_MaterialId;
typedef uint32_t   Obdn_S_TextureId;
typedef Mat4       Obdn_S_Xform;

#define OBDN_S_NONE (uint32_t)-1

typedef enum {
    OBDN_S_CAMERA_BIT      = 0x00000001,
    OBDN_S_LIGHTS_BIT      = 0x00000002,
    OBDN_S_XFORMS_BIT      = 0x00000004,
    OBDN_S_MATERIALS_BIT   = 0x00000008,
    OBDN_S_TEXTURES_BIT    = 0x00000010,
    OBDN_S_PRIMS_BIT       = 0x00000020,
    OBDN_S_WINDOW_BIT      = 0x00000040
} Obdn_S_DirtyBits;

typedef struct {
    Mat4 xform;
} Obdn_S_Camera;

typedef enum {
    OBDN_S_LIGHT_TYPE_POINT,
    OBDN_S_LIGHT_TYPE_DIRECTION
} Obdn_S_LightType;

typedef struct {
    Vec3 pos;
} Obdn_S_PointLight;

typedef struct {
    Vec3 dir;
} Obdn_S_DirectionLight;

typedef struct {
    Obdn_R_Primitive  rprim;
    Obdn_S_MaterialId materialId;
} Obdn_S_Primitive;

typedef union {
    Obdn_S_PointLight     pointLight;
    Obdn_S_DirectionLight directionLight;
} Obdn_S_LightStructure;

typedef struct {
    Obdn_S_LightStructure structure;
    float                  intensity;
    Vec3                   color;
    Obdn_S_LightType      type;
} Obdn_S_Light;

typedef struct {
    Obdn_V_Image        devImage;
    Obdn_V_BufferRegion hostBuffer;
} Obdn_S_Texture;

typedef struct {
    Vec3              color;
    float             roughness;
    Obdn_S_TextureId textureAlbedo;
    Obdn_S_TextureId textureRoughness;
    Obdn_S_TextureId textureNormal;
    Obdn_S_TextureId padding;
} Obdn_S_Material;

typedef struct {
    Obdn_S_PrimId     primCount;
    Obdn_S_LightId    lightCount;
    Obdn_S_MaterialId materialCount;
    Obdn_S_TextureId  textureCount;
    Obdn_S_Primitive  prims[OBDN_S_MAX_PRIMS];
    Obdn_S_Xform      xforms[OBDN_S_MAX_PRIMS];
    Obdn_S_Material   materials[OBDN_S_MAX_MATERIALS];
    Obdn_S_Texture    textures[OBDN_S_MAX_TEXTURES];
    Obdn_S_Light      lights[OBDN_S_MAX_LIGHTS];
    Obdn_S_Camera     camera;
    Obdn_S_DirtyMask  dirt;
} Obdn_S_Scene;

// container to associate prim ids with pipelines
typedef struct {
    Obdn_S_PrimId primCount;
    Obdn_S_PrimId primIds[OBDN_S_MAX_PRIMS];
} Obdn_S_PrimitiveList;

// counter-clockwise orientation
void obdn_s_CreateSimpleScene(Obdn_S_Scene* scene);
void obdn_s_UpdateCamera_ArcBall(Obdn_S_Scene* scene, float dt, int16_t mx, int16_t my, bool panning, bool tumbling, bool zooming, bool home);
void obdn_s_UpdateCamera_LookAt(Obdn_S_Scene* scene, Vec3 pos, Vec3 target, Vec3 up);
void obdn_s_CreateEmptyScene(Obdn_S_Scene* scene);
void obdn_s_UpdateLight(Obdn_S_Scene* scene, uint32_t id, float intensity);
void obdn_s_BindPrimToMaterial(Obdn_S_Scene* scene, const Obdn_S_PrimId primId, const Obdn_S_MaterialId matId);
void obdn_s_UpdatePrimXform(Obdn_S_Scene* scene, const Obdn_S_PrimId primId, const Mat4* delta);

void obdn_s_AddPrimToList(const Obdn_S_PrimId, Obdn_S_PrimitiveList*);
void obdn_s_ClearPrimList(Obdn_S_PrimitiveList*);

Obdn_S_PrimId    obdn_s_LoadPrim(Obdn_S_Scene* scene, const char* filePath, const Mat4* xform);
Obdn_S_PrimId    obdn_s_AddRPrim(Obdn_S_Scene* scene, const Obdn_R_Primitive prim, const Mat4* xform);
Obdn_S_TextureId obdn_s_LoadTexture(Obdn_S_Scene* scene, const char* filePath, const uint8_t channelCount);
Obdn_S_LightId   obdn_s_CreateDirectionLight(Obdn_S_Scene* scene, const Vec3 color, const Vec3 direction);
Obdn_S_LightId   obdn_s_CreatePointLight(Obdn_S_Scene* scene, const Vec3 color, const Vec3 position);
// a texture id of 0 means no texture will be used
Obdn_S_MaterialId obdn_s_CreateMaterial(Obdn_S_Scene* scene, Vec3 color, float roughness, 
        Obdn_S_TextureId albedoId, 
        Obdn_S_TextureId roughnessId,
        Obdn_S_TextureId normalId);

#endif /* end of include guard: OBDN_S_SCENE_H */
