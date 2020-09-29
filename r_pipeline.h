#ifndef R_PIPELINE_H
#define R_PIPELINE_H

#include "v_def.h"

#define MAX_DESCRIPTOR_SETS 3

extern VkPipeline pipelines[MAX_PIPELINES];
extern VkPipelineLayout pipelineLayoutRaster;
extern VkPipelineLayout pipelineLayoutRayTrace;
extern VkPipelineLayout pipelineLayoutPostProcess;

extern VkDescriptorSet descriptorSets[MAX_DESCRIPTOR_SETS];

typedef enum {
    R_PIPELINE_RASTER = 0,
    R_PIPELINE_RAYTRACE = 1,
    R_PIPELINE_POST = 2
} R_PipelineId;

typedef enum {
    R_DESC_SET_RASTER = 0,
    R_DESC_SET_RAYTRACE = 1,
    R_DESC_SET_POST = 2
} R_DescriptorSetId;

void initDescriptorSets(void);
void initPipelines(void);

void cleanUpPipelines(void);

#endif /* end of include guard: R_PIPELINE_H */

