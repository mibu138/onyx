#ifndef ONYX_R_PIPELINE_H
#define ONYX_R_PIPELINE_H

#include "def.h"
#include "geo.h"
#include "raytrace.h"

#define ONYX_MAX_PIPELINES 10
#define ONYX_MAX_DESCRIPTOR_SETS 100
#define ONYX_MAX_BINDINGS 10
#define ONYX_MAX_PUSH_CONSTANTS 5

#define ONYX_FULL_SCREEN_VERT_SPV "full-screen.vert.spv"

typedef enum Onyx_ShaderType {
    ONYX_SHADER_TYPE_VERTEX,
    ONYX_SHADER_TYPE_FRAGMENT,
    ONYX_SHADER_TYPE_COMPUTE,
    ONYX_SHADER_TYPE_GEOMETRY,
    ONYX_SHADER_TYPE_TESS_CONTROL,
    ONYX_SHADER_TYPE_TESS_EVALUATION,
    ONYX_SHADER_TYPE_RAY_GEN,
    ONYX_SHADER_TYPE_ANY_HIT,
    ONYX_SHADER_TYPE_CLOSEST_HIT,
    ONYX_SHADER_TYPE_MISS,
} Onyx_ShaderType;

typedef struct {
    uint32_t                 descriptorCount;
    VkDescriptorType         type;
    VkShaderStageFlags       stageFlags;
    VkDescriptorBindingFlags bindingFlags; //optional
} Onyx_DescriptorBinding;

typedef struct {
    size_t                  bindingCount;
    Onyx_DescriptorBinding* bindings;
} Onyx_DescriptorSetInfo;

typedef struct {
    VkDescriptorPool      descriptorPool;
    VkDescriptorSet       descriptorSets[ONYX_MAX_DESCRIPTOR_SETS];
    uint32_t              descriptorSetCount;
} Onyx_R_Description;

typedef struct {
    size_t descriptorSetCount;
    const VkDescriptorSetLayout* descriptorSetLayouts;
    size_t pushConstantCount;
    const VkPushConstantRange* pushConstantsRanges;
} Onyx_PipelineLayoutInfo;

typedef enum {
    ONYX_R_PIPELINE_RASTER_TYPE,
    ONYX_R_PIPELINE_RAYTRACE_TYPE
} Onyx_R_PipelineType;

typedef enum {
    ONYX_BLEND_MODE_NONE,
    ONYX_BLEND_MODE_OVER,
    ONYX_BLEND_MODE_OVER_NO_PREMUL, 
    ONYX_BLEND_MODE_ERASE,
    ONYX_BLEND_MODE_OVER_MONOCHROME,
    ONYX_BLEND_MODE_OVER_NO_PREMUL_MONOCHROME,
    ONYX_BLEND_MODE_ERASE_MONOCHROME,
} Onyx_BlendMode;

typedef struct {
    VkRenderPass              renderPass;
    VkPipelineLayout          layout;
    Onyx_VertexDescription  vertexDescription;
    VkPolygonMode             polygonMode;
    VkPrimitiveTopology       primitiveTopology;
    VkCullModeFlags           cullMode; // a value of 0 will default to culling the back faces
    VkFrontFace               frontFace;
    VkSampleCountFlags        sampleCount;
    Onyx_BlendMode          blendMode;
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
} Onyx_GraphicsPipelineInfo;

typedef struct {
    VkPipelineLayout layout;
    uint8_t          raygenCount;
    char**           raygenShaders;
    uint8_t          missCount;
    char**           missShaders;
    uint8_t          chitCount;
    char**           chitShaders;
} Onyx_RayTracePipelineInfo;

void onyx_DestroyDescription(VkDevice device, Onyx_R_Description*);

void onyx_CreateDescriptorSetLayout(
    VkDevice device,
    const uint8_t                  bindingCount,
    const Onyx_DescriptorBinding   bindings[/*bindingCount*/],
    VkDescriptorSetLayout*         layout);

void onyx_CreateDescriptorSetLayouts(VkDevice, const uint8_t count, const Onyx_DescriptorSetInfo sets[/*count*/],
        VkDescriptorSetLayout layouts[/*count*/]);
void onyx_CreateDescriptorSets(VkDevice device, const uint8_t count, const Onyx_DescriptorSetInfo sets[/*count*/], 
        const VkDescriptorSetLayout layouts[/*count*/],
        Onyx_R_Description* out);
void onyx_CreatePipelineLayouts(VkDevice, const uint8_t count, const Onyx_PipelineLayoutInfo layoutInfos[/*static count*/], 
        VkPipelineLayout pipelineLayouts[/*count*/]);
void onyx_CreateGraphicsPipelines(VkDevice device, const uint8_t count, const Onyx_GraphicsPipelineInfo pipelineInfos[/*count*/], VkPipeline pipelines[/*count*/]);
void onyx_CleanUpPipelines(void);
void onyx_CreateRayTracePipelines(VkDevice device, Onyx_Memory* memory, const uint8_t count, const Onyx_RayTracePipelineInfo pipelineInfos[/*count*/], 
        VkPipeline pipelines[/*count*/], Onyx_ShaderBindingTable shaderBindingTables[/*count*/]);

void onyx_CreateDescriptionsAndLayouts(VkDevice, const uint32_t descSetCount, const Onyx_DescriptorSetInfo sets[/*descSetCount*/], 
        VkDescriptorSetLayout layouts[/*descSetCount*/], 
        const uint32_t descriptionCount, Onyx_R_Description descriptions[/*descriptionCount*/]);

// very simple pipeline. counter clockwise. only considers position attribute (3 floats).
void onyx_CreateGraphicsPipeline_Taurus(VkDevice device, const VkRenderPass renderPass,
                                        const VkPipelineLayout layout,
                                        const VkPolygonMode mode,
                                        VkPipeline *pipeline);

void onyx_CreateDescriptorPool(VkDevice device,
                            uint32_t uniformBufferCount,
                            uint32_t dynamicUniformBufferCount,
                          uint32_t combinedImageSamplerCount,
                          uint32_t storageImageCount,
                          uint32_t storageBufferCount,
                          uint32_t inputAttachmentCount,
                          uint32_t accelerationStructureCount,
                          VkDescriptorPool* pool);

void onyx_AllocateDescriptorSets(VkDevice, VkDescriptorPool pool, uint32_t descSetCount,
                            const VkDescriptorSetLayout layouts[/*descSetCount*/],
                            VkDescriptorSet*      sets);

void onyx_SetRuntimeSpvPrefix(const char* prefix);

const char* onyx_GetFullScreenQuadShaderString(void);

#ifdef ONYX_SHADERC_ENABLED
void onyx_CreateShaderModule(VkDevice device, const char* shader_string, 
        const char* name, Onyx_ShaderType type, VkShaderModule* module);
#endif

// Creates a basic graphics pipeline.
// patchControlPoints will be ignored unless a tesselation shader stage is
// passed in. If dynamic_viewport is enabled, it doesnt matter what you put for
// viewport_width or viewport_height.
void onyx_CreateGraphicsPipeline_Basic(VkDevice device, VkPipelineLayout layout,
        VkRenderPass renderPass, uint32_t subpass, 
        uint32_t stageCount, const VkPipelineShaderStageCreateInfo* pStages,
        const VkPipelineVertexInputStateCreateInfo* pVertexInputState,
        VkPrimitiveTopology topology, uint32_t patchControlPoints, uint32_t viewport_width,
        uint32_t viewport_height, bool dynamic_viewport,
        VkPolygonMode polygonMode, VkFrontFace frontFace, float lineWidth, bool depthTestEnable, 
        bool depthWriteEnable,
        Onyx_BlendMode blendMode,
        VkPipeline* pipeline);

#endif /* end of include guard: R_PIPELINE_H */

