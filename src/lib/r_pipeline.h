#ifndef OBDN_R_PIPELINE_H
#define OBDN_R_PIPELINE_H

#include "v_def.h"
#include "r_geo.h"
#include "r_raytrace.h"

#define OBDN_MAX_PIPELINES 10
#define OBDN_MAX_DESCRIPTOR_SETS 10
#define OBDN_MAX_BINDINGS 10
#define OBDN_MAX_PUSH_CONSTANTS 5

#define OBDN_FULL_SCREEN_VERT_SPV "full-screen.vert.spv"


typedef struct {
    uint32_t                 descriptorCount;
    VkDescriptorType         type;
    VkShaderStageFlags       stageFlags;
    VkDescriptorBindingFlags bindingFlags; //optional
} Obdn_R_DescriptorBinding;

typedef struct {
    size_t                   bindingCount;
    Obdn_R_DescriptorBinding bindings[OBDN_MAX_BINDINGS];
} Obdn_R_DescriptorSetInfo;

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
} Obdn_R_PipelineLayoutInfo;

typedef enum {
    OBDN_R_PIPELINE_RASTER_TYPE,
    OBDN_R_PIPELINE_RAYTRACE_TYPE
} Obdn_R_PipelineType;

typedef enum {
    OBDN_R_BLEND_MODE_NONE,
    OBDN_R_BLEND_MODE_OVER,
    OBDN_R_BLEND_MODE_OVER_STRAIGHT, //no premultiply
    OBDN_R_BLEND_MODE_ERASE,
} Obdn_R_BlendMode;

typedef struct {
    VkRenderPass              renderPass;
    VkPipelineLayout          layout;
    Obdn_R_VertexDescription  vertexDescription;
    VkPolygonMode             polygonMode;
    VkCullModeFlags           cullMode; // a value of 0 will default to culling the back faces
    VkFrontFace               frontFace;
    VkSampleCountFlags        sampleCount;
    Obdn_R_BlendMode          blendMode;
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
} Obdn_R_GraphicsPipelineInfo;

typedef struct {
    VkPipelineLayout layout;
    uint8_t          raygenCount;
    char**           raygenShaders;
    uint8_t          missCount;
    char**           missShaders;
    uint8_t          chitCount;
    char**           chitShaders;
} Obdn_R_RayTracePipelineInfo;

void obdn_DestroyDescription(VkDevice device, Obdn_R_Description*);

void obdn_r_CreateDescriptorSetLayout(
    const uint8_t                  bindingCount,
    const Obdn_R_DescriptorBinding bindings[bindingCount],
    VkDescriptorSetLayout*         layout);

void obdn_CreateDescriptorSetLayouts(VkDevice, const uint8_t count, const Obdn_R_DescriptorSetInfo sets[count],
        VkDescriptorSetLayout layouts[count]);
void obdn_CreateDescriptorSets(VkDevice device, const uint8_t count, const Obdn_R_DescriptorSetInfo sets[count], 
        const VkDescriptorSetLayout layouts[count],
        Obdn_R_Description* out);
void obdn_CreatePipelineLayouts(VkDevice, const uint8_t count, const Obdn_R_PipelineLayoutInfo layoutInfos[static count], 
        VkPipelineLayout pipelineLayouts[count]);
void obdn_CreateGraphicsPipelines(VkDevice device, const uint8_t count, const Obdn_R_GraphicsPipelineInfo pipelineInfos[count], VkPipeline pipelines[count]);
void obdn_CleanUpPipelines(void);
void obdn_CreateRayTracePipelines(VkDevice device, Obdn_Memory* memory, const uint8_t count, const Obdn_R_RayTracePipelineInfo pipelineInfos[count], 
        VkPipeline pipelines[count], Obdn_R_ShaderBindingTable shaderBindingTables[count]);

void obdn_CreateDescriptionsAndLayouts(VkDevice, const uint32_t descSetCount, const Obdn_R_DescriptorSetInfo sets[descSetCount], 
        VkDescriptorSetLayout layouts[descSetCount], 
        const uint32_t descriptionCount, Obdn_R_Description descriptions[descSetCount]);

// very simple pipeline. counter clockwise. only considers position attribute (3 floats).
void obdn_CreateGraphicsPipeline_Taurus(VkDevice device, const VkRenderPass renderPass,
                                        const VkPipelineLayout layout,
                                        const VkPolygonMode mode,
                                        VkPipeline *pipeline);

void obdn_CreateDescriptorPool(VkDevice, uint32_t uniformBufferCount,
                          uint32_t combinedImageSamplerCount,
                          uint32_t storageImageCount,
                          uint32_t storageBufferCount,
                          uint32_t inputAttachmentCount,
                          uint32_t accelerationStructureCount,
                          VkDescriptorPool* pool);

void obdn_AllocateDescriptorSets(VkDevice, VkDescriptorPool pool, uint32_t descSetCount,
                            const VkDescriptorSetLayout layouts[descSetCount],
                            VkDescriptorSet*      sets);

#endif /* end of include guard: R_PIPELINE_H */

