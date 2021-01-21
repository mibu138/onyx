#ifndef TANTO_S_SCENE_H
#define TANTO_S_SCENE_H

#include "r_geo.h"
#include "t_def.h"

#define TANTO_S_MAX_PRIMS     256 
#define TANTO_S_MAX_MATERIALS 16
#define TANTO_S_MAX_LIGHTS    16
#define TANTO_S_MAX_TEXTURES  16

typedef Tanto_Mask Tanto_S_DirtyMask;
typedef uint32_t   Tanto_S_PrimId;
typedef uint32_t   Tanto_S_LightId;
typedef uint32_t   Tanto_S_MaterialId;
typedef uint32_t   Tanto_S_TextureId;
typedef Mat4       Tanto_S_Xform;

#define TANTO_S_NONE (uint32_t)-1


typedef enum {
    TANTO_S_CAMERA_BIT    = 0x00000001,
    TANTO_S_LIGHTS_BIT    = 0x00000002,
    TANTO_S_XFORMS_BIT    = 0x00000004,
    TANTO_S_MATERIALS_BIT = 0x00000008,
    TANTO_S_TEXTURES_BIT  = 0x00000010,
} Tanto_S_DirtyBits;

typedef struct {
    Mat4 xform;
} Tanto_S_Camera;

typedef enum {
    TANTO_S_LIGHT_TYPE_POINT,
    TANTO_S_LIGHT_TYPE_DIRECTION
} Tanto_S_LightType;

typedef struct {
    Vec3 pos;
} Tanto_S_PointLight;

typedef struct {
    Vec3 dir;
} Tanto_S_DirectionLight;

typedef struct {
    Tanto_R_Primitive  rprim;
    Tanto_S_MaterialId materialId;
} Tanto_S_Primitive;

typedef union {
    Tanto_S_PointLight     pointLight;
    Tanto_S_DirectionLight directionLight;
} Tanto_S_LightStructure;

typedef struct {
    Tanto_S_LightStructure structure;
    float                  intensity;
    Vec3                   color;
    Tanto_S_LightType      type;
} Tanto_S_Light;

typedef struct {
    Tanto_V_Image        devImage;
    Tanto_V_BufferRegion hostBuffer;
} Tanto_S_Texture;

typedef struct {
    Vec3              color;
    float             roughness;
    Tanto_S_TextureId textureAlbedo;
    Tanto_S_TextureId textureRoughness;
} Tanto_S_Material;

typedef struct {
    Tanto_S_PrimId     primCount;
    Tanto_S_LightId    lightCount;
    Tanto_S_MaterialId materialCount;
    Tanto_S_TextureId  textureCount;
    Tanto_S_Primitive  prims[TANTO_S_MAX_PRIMS];
    Tanto_S_Xform      xforms[TANTO_S_MAX_PRIMS];
    Tanto_S_Material   materials[TANTO_S_MAX_MATERIALS];
    Tanto_S_Texture    textures[TANTO_S_MAX_TEXTURES];
    Tanto_S_Light      lights[TANTO_S_MAX_LIGHTS];
    Tanto_S_Camera     camera;
    Tanto_S_DirtyMask  dirt;
} Tanto_S_Scene;

// counter-clockwise orientation
void tanto_s_CreateSimpleScene(Tanto_S_Scene* scene);
void tanto_s_UpdateCamera_ArcBall(Tanto_S_Scene* scene, float dt, int16_t mx, int16_t my, bool panning, bool tumbling, bool zooming, bool home);
void tanto_s_UpdateCamera_LookAt(Tanto_S_Scene* scene, Vec3 pos, Vec3 target, Vec3 up);
void tanto_s_CreateEmptyScene(Tanto_S_Scene* scene);
void tanto_s_UpdateLight(Tanto_S_Scene* scene, uint32_t id, float intensity);
void tanto_s_BindPrimToMaterial(Tanto_S_Scene* scene, const Tanto_S_PrimId primId, const Tanto_S_MaterialId matId);

Tanto_S_PrimId    tanto_s_LoadPrim(Tanto_S_Scene* scene, const char* filePath, const Mat4* xform);
Tanto_S_PrimId    tanto_s_AddRPrim(Tanto_S_Scene* scene, const Tanto_R_Primitive prim, const Mat4* xform);
Tanto_S_TextureId tanto_s_LoadTexture(Tanto_S_Scene* scene, const char* filePath, const uint8_t channelCount);
Tanto_S_LightId   tanto_s_CreateDirectionLight(Tanto_S_Scene* scene, const Vec3 color, const Vec3 direction);
Tanto_S_LightId   tanto_s_CreatePointLight(Tanto_S_Scene* scene, const Vec3 color, const Vec3 position);
Tanto_S_MaterialId tanto_s_CreateMaterial(Tanto_S_Scene* scene, Vec3 color, float roughness, Tanto_S_TextureId albedoId, Tanto_S_TextureId roughnessId);

#endif /* end of include guard: TANTO_S_SCENE_H */
