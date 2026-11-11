#pragma once

#include "gpu_shader_module.h"
#include "gpu_descriptor_pool.h"
#include <vulkan/vulkan.h>
#include <array>
#include <vector>

// Descriptor set 0/1/2 are vertex/fragment/compute respectively.
struct PGAndroidPipelineLayout {
    std::array<VkDescriptorSetLayout,3> setLayouts{};
    std::array<VkDescriptorSet,3> descriptorSets{};
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::array<std::vector<PGAndroidDescriptorBinding>,3> bindings;
};

bool PGAndroidCreatePipelineLayout(VkDevice device, PGAndroidDescriptorPool &pool,
    const PGAndroidShaderModule *shaders, uint32_t shaderCount,
    PGAndroidDescriptorWriter &writer, PGAndroidPipelineLayout &out);
bool PGAndroidUpdatePipelineDescriptors(VkDevice device, const PGAndroidPipelineLayout &layout,
    const PGAndroidShaderModule *shaders, uint32_t shaderCount,
    const PGAndroidGpuBindingState &state, const PGAndroidGpuResources &resources,
    PGAndroidDescriptorWriter &writer);
void PGAndroidDestroyPipelineLayout(VkDevice device, PGAndroidDescriptorPool &pool,
    PGAndroidPipelineLayout &layout);
