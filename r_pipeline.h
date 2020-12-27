#ifndef TANTO_R_PIPELINE_H
#define TANTO_R_PIPELINE_H

#include "v_def.h"
#include "r_geo.h"

#define TANTO_MAX_PIPELINES 10
#define TANTO_MAX_DESCRIPTOR_SETS 10
#define TANTO_MAX_BINDINGS 10
#define TANTO_MAX_PUSH_CONSTANTS 5

#define TANTO_SPVDIR "./tanto/shaders/spv"

typedef struct {
    uint32_t                 descriptorCount;
    VkDescriptorType         type;
    VkShaderStageFlags       stageFlags;
    VkDescriptorBindingFlags bindingFlags; //optional
} Tanto_R_DescriptorBinding;

typedef struct {
    size_t                    bindingCount;
    Tanto_R_DescriptorBinding bindings[TANTO_MAX_BINDINGS];
} Tanto_R_DescriptorSetInfo;

typedef struct {
    VkDescriptorPool      descriptorPool;
    VkDescriptorSetLayout descriptorSetLayouts[TANTO_MAX_DESCRIPTOR_SETS]; 
    VkDescriptorSet       descriptorSets[TANTO_MAX_DESCRIPTOR_SETS];
    uint32_t              descriptorSetCount;
} Tanto_R_Description;

typedef struct {
    size_t descriptorSetCount;
    const VkDescriptorSetLayout* descriptorSetLayouts;
    size_t pushConstantCount;
    const VkPushConstantRange* pushConstantsRanges;
} Tanto_R_PipelineLayoutInfo;

typedef enum {
    TANTO_R_PIPELINE_RASTER_TYPE,
    TANTO_R_PIPELINE_RAYTRACE_TYPE
} Tanto_R_PipelineType;

typedef enum {
    TANTO_R_BLEND_MODE_NONE,
    TANTO_R_BLEND_MODE_OVER,
    TANTO_R_BLEND_MODE_ERASE,
} Tanto_R_BlendMode;

typedef struct {
    VkRenderPass              renderPass;
    VkPipelineLayout          layout;
    Tanto_R_VertexDescription vertexDescription;
    VkPolygonMode             polygonMode;
    VkCullModeFlags           cullMode; // a value of 0 will default to culling the back faces
    VkFrontFace               frontFace;
    VkSampleCountFlags        sampleCount;
    Tanto_R_BlendMode         blendMode;
    VkExtent2D                viewportDim;
    uint32_t                  tesselationPatchPoints;
    char* vertShader;
    char* fragShader;
    char* tessCtrlShader;
    char* tessEvalShader;
} Tanto_R_GraphicsPipelineInfo;

typedef struct {
    VkPipelineLayout layout;
    uint8_t          raygenCount;
    char**           raygenShaders;
    uint8_t          missCount;
    char**           missShaders;
    uint8_t          chitCount;
    char**           chitShaders;
} Tanto_R_RayTracePipelineInfo;

union Tanto_R_PipelineInfoPayload {
    Tanto_R_GraphicsPipelineInfo rasterInfo;
    Tanto_R_RayTracePipelineInfo rayTraceInfo;
};

typedef struct {
    int   id;
    Tanto_R_PipelineType type;
    int   layoutId;
    union Tanto_R_PipelineInfoPayload payload;
} Tanto_R_PipelineInfo;

void tanto_r_CreateDescriptorSets(const uint8_t count, const Tanto_R_DescriptorSetInfo sets[count],
        Tanto_R_Description* out);
void tanto_r_CreatePipelineLayouts(const uint8_t count, const Tanto_R_PipelineLayoutInfo layoutInfos[static count], 
        VkPipelineLayout pipelineLayouts[count]);
void tanto_r_InitPipelines(const Tanto_R_PipelineInfo* const pipelineInfos, const int count);
void tanto_r_CreateGraphicsPipelines(const uint8_t count, const Tanto_R_GraphicsPipelineInfo pipelineInfos[count], VkPipeline pipelines[count]);
void tanto_r_CreateRayTracePipelines(const uint8_t count, const Tanto_R_RayTracePipelineInfo pipelineInfos[count], VkPipeline pipelines[count]);
void tanto_r_CreatePipeline(const Tanto_R_PipelineInfo* const pipelineInfo, VkPipeline* pPipeline);
void tanto_r_CleanUpPipelines(void);

// has clockwise orientation
char* tanto_r_FullscreenTriVertShader(void);

#endif /* end of include guard: R_PIPELINE_H */

