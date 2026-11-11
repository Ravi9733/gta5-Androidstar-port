#pragma once

#include "gpu_pipeline_layout.h"

#include <vulkan/vulkan.h>
#include <cstdint>

struct PGAndroidPipeline {
    VkPipeline handle = VK_NULL_HANDLE;
    bool compute = false;
};

// The caller owns all pointed-to create-info arrays for the duration of the call.
// The wrapper validates stage/module agreement and supplies the Android pipeline layout.
bool PGAndroidCreateGraphicsPipeline(VkDevice device, VkPipelineCache cache,
    const PGAndroidPipelineLayout &layout, VkRenderPass renderPass,
    const PGAndroidShaderModule *shaders, uint32_t shaderCount,
    const VkGraphicsPipelineCreateInfo &createInfo, PGAndroidPipeline &out);

bool PGAndroidCreateComputePipeline(VkDevice device, VkPipelineCache cache,
    const PGAndroidPipelineLayout &layout, const PGAndroidShaderModule &shader,
    const VkComputePipelineCreateInfo &createInfo, PGAndroidPipeline &out);

void PGAndroidDestroyPipeline(VkDevice device, PGAndroidPipeline &pipeline);
