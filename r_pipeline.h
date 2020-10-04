#ifndef R_PIPELINE_H
#define R_PIPELINE_H

#include "v_def.h"

#define MAX_PIPELINES 10
#define MAX_DESCRIPTOR_SETS 10
#define MAX_BINDINGS 10
#define MAX_PUSH_CONSTANTS 5

#define SPVDIR "/home/michaelb/dev/tanto/shaders/spv"

extern VkPipeline       pipelines[MAX_PIPELINES];
extern VkDescriptorSet  descriptorSets[MAX_DESCRIPTOR_SETS];
extern VkPipelineLayout pipelineLayouts[MAX_PIPELINES];

typedef struct {
    uint32_t           descriptorCount;
    VkDescriptorType   type;
    VkShaderStageFlags stageFlags;
} R_DescriptorBinding;

typedef struct {
    int    id;
    size_t bindingCount;
    R_DescriptorBinding bindings[MAX_BINDINGS];
} R_DescriptorSet;

typedef struct {
    int id;
    size_t descriptorSetCount;
    int    descriptorSetIds[MAX_DESCRIPTOR_SETS];
    size_t pushConstantCount;
    VkPushConstantRange pushConstantsRanges[MAX_PUSH_CONSTANTS];
} R_PipelineLayout;

typedef enum {
    R_PIPELINE_TYPE_RASTER,
    R_PIPELINE_TYPE_RAYTRACE,
    R_PIPELINE_TYPE_POSTPROC
} R_PipelineType;

typedef enum {
    R_RENDER_PASS_TYPE_SWAPCHAIN,
    R_RENDER_PASS_TYPE_OFFSCREEN
} R_RenderPassType;

typedef struct {
    const R_RenderPassType renderPassType;
    const char* vertShader;
    const char* fragShader;
} R_PipelineRasterInfo;

typedef struct {
} R_PipelineRayTraceInfo;

typedef struct {
    const int   id;
    const R_PipelineType type;
    const int   pipelineLayoutId;
    const R_PipelineRasterInfo rasterInfo;
    const R_PipelineRayTraceInfo raytraceInfo;
} R_PipelineInfo;


void r_InitDescriptorSets(const R_DescriptorSet* const sets, const int count);
void r_InitPipelineLayouts(const R_PipelineLayout* const layouts, const int count);
void r_InitPipelines(const R_PipelineInfo* const pipelineInfos, const int count);
//void initPipelines(void);

void cleanUpPipelines(void);

#endif /* end of include guard: R_PIPELINE_H */

