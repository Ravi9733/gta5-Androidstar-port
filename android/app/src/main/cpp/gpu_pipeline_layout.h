#pragma once

#include "gpu_shader_module.h"
#include "gpu_descriptor_pool.h"

#include <vulkan/vulkan.h>
#include <vector>

struct PGAndroidPipelineLayout {
    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    std::vector<PGAndroidDescriptorBinding> bindings;
};

bool PGAndroidCreatePipelineLayout(VkDevice device, PGAndroidDescriptorPool &pool,
                                   const PGAndroidShaderModule *shaders, uint32_t shaderCount,
                                   PGAndroidDescriptorWriter &writer,
                                   PGAndroidPipelineLayout &out);
void PGAndroidDestroyPipelineLayout(VkDevice device, PGAndroidDescriptorPool &pool,
                                    PGAndroidPipelineLayout &layout);
