#pragma once

#include "gpu_shader_bindings.h"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <vector>

struct PGAndroidShaderModule {
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderStageFlagBits stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    uint32_t stageIndex = 0;
    std::vector<PGAndroidShaderResource> reflection;
    std::vector<PGAndroidDescriptorBinding> descriptors;
};

// Creates a Vulkan shader module from SPIR-V and keeps the original iOS reflection
// mapped to descriptor bindings. SPIR-V generation itself is a separate build step.
bool PGAndroidCreateShaderModule(VkDevice device, const uint32_t *spirv, size_t byteCount,
                                 VkShaderStageFlagBits stage,
                                 const PGAndroidShaderResource *resources, uint32_t resourceCount,
                                 const PGAndroidGpuBindingState &state,
                                 PGAndroidShaderModule &out);
void PGAndroidDestroyShaderModule(VkDevice device, PGAndroidShaderModule &shader);
