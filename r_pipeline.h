#ifndef R_PIPELINE_H
#define R_PIPELINE_H

#include "v_def.h"

typedef enum {
    R_PIPE_RASTER,
    R_PIPE_RAYTRACE,
    R_PIPE_POST,
    R_PIPE_ID_SIZE
} R_PipelineId;

typedef enum {
    R_PIPE_LAYOUT_RASTER,
    R_PIPE_LAYOUT_RAYTRACE,
    R_PIPE_LAYOUT_POST,
    R_PIPE_LAYOUT_ID_SIZE
} R_PipelineLayoutId;

typedef enum {
    R_DESC_SET_RASTER,
    R_DESC_SET_RAYTRACE,
    R_DESC_SET_POST,
    R_DESC_SET_ID_SIZE
} R_DescriptorSetId;

extern VkPipeline       pipelines[R_PIPE_ID_SIZE];
extern VkDescriptorSet  descriptorSets[R_DESC_SET_ID_SIZE];
extern VkPipelineLayout pipelineLayouts[R_PIPE_LAYOUT_ID_SIZE];

void initDescriptorSets(void);
void initPipelines(void);

void cleanUpPipelines(void);

#endif /* end of include guard: R_PIPELINE_H */

