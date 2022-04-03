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

typedef enum OnyxBlendMode {
    ONYX_BLEND_MODE_NONE,
    ONYX_BLEND_MODE_OVER,
    ONYX_BLEND_MODE_OVER_NO_PREMUL, 
    ONYX_BLEND_MODE_ERASE,
    ONYX_BLEND_MODE_OVER_MONOCHROME,
    ONYX_BLEND_MODE_OVER_NO_PREMUL_MONOCHROME,
    ONYX_BLEND_MODE_ERASE_MONOCHROME,
} OnyxBlendMode;

// remove eventually
typedef OnyxBlendMode Onyx_BlendMode;

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

typedef struct OnyxPipelineShaderStageInfo {
    VkPipelineShaderStageCreateFlags    flags;
    VkShaderStageFlagBits               stage;
    VkShaderModule                      module;
    const char*                         p_name;
    const VkSpecializationInfo*         p_specialization_info;
} OnyxPipelineShaderStageInfo;

// 1 less than VkCompareOp... because 0 as none is a bad default
typedef enum OnyxCompareOp {
    ONYX_COMPARE_OP_LESS  = 0,
    ONYX_COMPARE_OP_EQUAL = 1,
    ONYX_COMPARE_OP_LESS_OR_EQUAL = 2,
    ONYX_COMPARE_OP_GREATER = 3,
    ONYX_COMPARE_OP_NOT_EQUAL = 4,
    ONYX_COMPARE_OP_GREATER_OR_EQUAL = 5,
    ONYX_COMPARE_OP_ALWAYS = 6,
} OnyxCompareOp;

typedef struct OnyxPipelineColorBlendAttachment {
    bool          blend_enable;
    OnyxBlendMode blend_mode;
} OnyxPipelineColorBlendAttachment;

_Static_assert(ONYX_COMPARE_OP_LESS == VK_COMPARE_OP_LESS - 1, "Should be 1 less");

typedef struct OnyxGraphicsPipelineInfo {
    VkDevice         device;
    VkPipelineLayout layout;
    VkRenderPass     render_pass;
    uint32_t         subpass;

    const VkPipeline      base_pipeline;
    uint32_t              base_pipeline_index;

    uint32_t                           shader_stage_count;
    const OnyxPipelineShaderStageInfo* shader_stages;

    // from VkPipelineInputAssemblyStateCreateInfo
    VkPrimitiveTopology topology;
    bool                primitive_restart_enable;

    // from VkPipelineVertexInputStateCreateInfo
    uint32_t                                 vertex_binding_description_count;
    const VkVertexInputBindingDescription*   vertex_binding_descriptions;
    uint32_t                                 vertex_attribute_description_count;
    const VkVertexInputAttributeDescription* vertex_attribute_descriptions;
    uint32_t                                 patch_control_points;

    // from VkPipelineViewportStateCreateInfo
    // can be ignored if both scissor and viewport are dynamic
    uint32_t viewport_width;
    uint32_t viewport_height;

    // from VkPipelineRasterizationStateCreateInfo
    bool            depth_clamp_enable;
    bool            rasterizer_discard_enable;
    // 0 == fill
    VkPolygonMode   polygon_mode;
    VkCullModeFlags cull_mode;
    // 0 == counter clock wise
    VkFrontFace     front_face;
    bool            depth_bias_enable;
    float           depth_bias_constant_factor;
    float           depth_bias_clamp;
    float           depth_bias_slope_factor;
    float           line_width;

    // from VkPipelineMultisampleStateCreateInfo
    // if this is 0, it will automatically be set to VK_SAMPLE_COUNT_1_BIT
    VkSampleCountFlags  rasterization_samples;
    // these can all be left at 0
    VkBool32            sample_shading_enable;
    float               min_sample_shading;
    const VkSampleMask* p_sample_mask;
    VkBool32            alpha_to_coverage_enable;
    VkBool32            alpha_to_one_enable;

    // from VkPipelineDepthStencilStateCreateInfo
    bool             depth_test_enable;
    bool             depth_write_enable;
    OnyxCompareOp    depth_compare_op;
    bool             depth_bounds_test_enable;
    bool             stencil_test_enable;
    VkStencilOpState front;
    VkStencilOpState back;
    float            min_depth_bounds;
    float            max_depth_bounds;

    // from VkPipelineColorBlendStateCreateInfo
    // if logic op is enabled, the operation applies to all attachments
    // and color blending is disabled. only valid for integer attachments.
    bool                              logic_op_enable;
    VkLogicOp                         logic_op;
    uint32_t                          attachment_count;
    OnyxPipelineColorBlendAttachment* attachment_blends;
    float                             blend_constants[4];

    //
    uint32_t        dynamic_state_count;
    VkDynamicState* dynamic_states;
} OnyxGraphicsPipelineInfo;

typedef struct OnyxComputePipelineInfo {
    VkPipelineCreateFlags       flags;
    VkPipelineLayout            layout;
    OnyxPipelineShaderStageInfo shader_stage;
    VkPipeline                  base_pipeline;
    int32_t                     base_pipeline_index;
} OnyxComputePipelineInfo;

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

void
onyx_create_graphics_pipeline(const OnyxGraphicsPipelineInfo* s,
                              const VkPipelineCache cache, VkPipeline* pipeline);

void
onyx_create_compute_pipeline(VkDevice, VkPipelineCache, const OnyxComputePipelineInfo* s,
                            VkPipeline* pipeline);

void onyx_create_pipeline_layout(VkDevice device, const Onyx_PipelineLayoutInfo* li,
        VkPipelineLayout* layout);

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

void onyx_CreateShaderModule(VkDevice device, const char* shader_string, 
        const char* name, Onyx_ShaderType type, VkShaderModule* module);

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

