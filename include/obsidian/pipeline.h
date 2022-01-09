#ifndef OBDN_R_PIPELINE_H
#define OBDN_R_PIPELINE_H

#include "def.h"
#include "geo.h"
#include "raytrace.h"

#define OBDN_MAX_PIPELINES 10
#define OBDN_MAX_DESCRIPTOR_SETS 100
#define OBDN_MAX_BINDINGS 10
#define OBDN_MAX_PUSH_CONSTANTS 5

#define OBDN_FULL_SCREEN_VERT_SPV "full-screen.vert.spv"

typedef enum Obdn_ShaderType {
    OBDN_SHADER_TYPE_VERTEX,
    OBDN_SHADER_TYPE_FRAGMENT,
    OBDN_SHADER_TYPE_COMPUTE,
    OBDN_SHADER_TYPE_GEOMETRY,
    OBDN_SHADER_TYPE_TESS_CONTROL,
    OBDN_SHADER_TYPE_TESS_EVALUATION,
    OBDN_SHADER_TYPE_RAY_GEN,
    OBDN_SHADER_TYPE_ANY_HIT,
    OBDN_SHADER_TYPE_CLOSEST_HIT,
    OBDN_SHADER_TYPE_MISS,
} Obdn_ShaderType;

typedef struct {
    uint32_t                 descriptorCount;
    VkDescriptorType         type;
    VkShaderStageFlags       stageFlags;
    VkDescriptorBindingFlags bindingFlags; //optional
} Obdn_DescriptorBinding;

typedef struct {
    size_t                  bindingCount;
    Obdn_DescriptorBinding* bindings;
} Obdn_DescriptorSetInfo;

typedef struct {
    VkDescriptorPool      descriptorPool;
    VkDescriptorSet       descriptorSets[OBDN_MAX_DESCRIPTOR_SETS];
    uint32_t              descriptorSetCount;
} Obdn_R_Description;

typedef struct {
    size_t descriptorSetCount;
    const VkDescriptorSetLayout* descriptorSetLayouts;
    size_t pushConstantCount;
    const VkPushConstantRange* pushConstantsRanges;
} Obdn_PipelineLayoutInfo;

typedef enum {
    OBDN_R_PIPELINE_RASTER_TYPE,
    OBDN_R_PIPELINE_RAYTRACE_TYPE
} Obdn_R_PipelineType;

typedef enum {
    OBDN_BLEND_MODE_NONE,
    OBDN_BLEND_MODE_OVER,
    OBDN_BLEND_MODE_OVER_NO_PREMUL, 
    OBDN_BLEND_MODE_ERASE,
    OBDN_BLEND_MODE_OVER_MONOCHROME,
    OBDN_BLEND_MODE_OVER_NO_PREMUL_MONOCHROME,
    OBDN_BLEND_MODE_ERASE_MONOCHROME,
} Obdn_BlendMode;

typedef struct {
    VkRenderPass              renderPass;
    VkPipelineLayout          layout;
    Obdn_VertexDescription  vertexDescription;
    VkPolygonMode             polygonMode;
    VkPrimitiveTopology       primitiveTopology;
    VkCullModeFlags           cullMode; // a value of 0 will default to culling the back faces
    VkFrontFace               frontFace;
    VkSampleCountFlags        sampleCount;
    Obdn_BlendMode          blendMode;
    VkExtent2D                viewportDim;
    uint32_t                  attachmentCount;
    uint32_t                  tesselationPatchPoints;
    uint32_t                  subpass;
    VkSpecializationInfo*     pVertSpecializationInfo;
    VkSpecializationInfo*     pFragSpecializationInfo;
    char*                     vertShader;
    char*                     fragShader;
    char*                     tessCtrlShader;
    char*                     tessEvalShader;
    uint32_t                  dynamicStateCount;
    VkDynamicState*           pDynamicStates;
} Obdn_GraphicsPipelineInfo;

typedef struct {
    VkPipelineLayout layout;
    uint8_t          raygenCount;
    char**           raygenShaders;
    uint8_t          missCount;
    char**           missShaders;
    uint8_t          chitCount;
    char**           chitShaders;
} Obdn_RayTracePipelineInfo;

void obdn_DestroyDescription(VkDevice device, Obdn_R_Description*);

void obdn_CreateDescriptorSetLayout(
    VkDevice device,
    const uint8_t                  bindingCount,
    const Obdn_DescriptorBinding   bindings[/*bindingCount*/],
    VkDescriptorSetLayout*         layout);

void obdn_CreateDescriptorSetLayouts(VkDevice, const uint8_t count, const Obdn_DescriptorSetInfo sets[/*count*/],
        VkDescriptorSetLayout layouts[/*count*/]);
void obdn_CreateDescriptorSets(VkDevice device, const uint8_t count, const Obdn_DescriptorSetInfo sets[/*count*/], 
        const VkDescriptorSetLayout layouts[/*count*/],
        Obdn_R_Description* out);
void obdn_CreatePipelineLayouts(VkDevice, const uint8_t count, const Obdn_PipelineLayoutInfo layoutInfos[/*static count*/], 
        VkPipelineLayout pipelineLayouts[/*count*/]);
void obdn_CreateGraphicsPipelines(VkDevice device, const uint8_t count, const Obdn_GraphicsPipelineInfo pipelineInfos[/*count*/], VkPipeline pipelines[/*count*/]);
void obdn_CleanUpPipelines(void);
void obdn_CreateRayTracePipelines(VkDevice device, Obdn_Memory* memory, const uint8_t count, const Obdn_RayTracePipelineInfo pipelineInfos[/*count*/], 
        VkPipeline pipelines[/*count*/], Obdn_ShaderBindingTable shaderBindingTables[/*count*/]);

void obdn_CreateDescriptionsAndLayouts(VkDevice, const uint32_t descSetCount, const Obdn_DescriptorSetInfo sets[/*descSetCount*/], 
        VkDescriptorSetLayout layouts[/*descSetCount*/], 
        const uint32_t descriptionCount, Obdn_R_Description descriptions[/*descriptionCount*/]);

// very simple pipeline. counter clockwise. only considers position attribute (3 floats).
void obdn_CreateGraphicsPipeline_Taurus(VkDevice device, const VkRenderPass renderPass,
                                        const VkPipelineLayout layout,
                                        const VkPolygonMode mode,
                                        VkPipeline *pipeline);

void obdn_CreateDescriptorPool(VkDevice device,
                            uint32_t uniformBufferCount,
                            uint32_t dynamicUniformBufferCount,
                          uint32_t combinedImageSamplerCount,
                          uint32_t storageImageCount,
                          uint32_t storageBufferCount,
                          uint32_t inputAttachmentCount,
                          uint32_t accelerationStructureCount,
                          VkDescriptorPool* pool);

void obdn_AllocateDescriptorSets(VkDevice, VkDescriptorPool pool, uint32_t descSetCount,
                            const VkDescriptorSetLayout layouts[/*descSetCount*/],
                            VkDescriptorSet*      sets);

void obdn_SetRuntimeSpvPrefix(const char* prefix);

const char* obdn_GetFullScreenQuadShaderString(void);

#ifdef OBSIDIAN_SHADERC_ENABLED
void obdn_CreateShaderModule(VkDevice device, const char* shader_string, 
        const char* name, Obdn_ShaderType type, VkShaderModule* module);
#endif

// Creates a basic graphics pipeline.
// patchControlPoints will be ignored unless a tesselation shader stage is
// passed in. If dynamic_viewport is enabled, it doesnt matter what you put for
// viewport_width or viewport_height.
void obdn_CreateGraphicsPipeline_Basic(VkDevice device, VkPipelineLayout layout,
        VkRenderPass renderPass, uint32_t subpass, 
        uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages,
        const VkPipelineVertexInputStateCreateInfo* pVertexInputState,
        VkPrimitiveTopology topology, uint32_t patchControlPoints, uint32_t viewport_width,
        uint32_t viewport_height, bool dynamic_viewport,
        VkPolygonMode polygonMode, VkFrontFace frontFace, float lineWidth, bool depthTestEnable, 
        bool depthWriteEnable,
        Obdn_BlendMode blendMode,
        VkPipeline* pipeline);

#endif /* end of include guard: R_PIPELINE_H */

