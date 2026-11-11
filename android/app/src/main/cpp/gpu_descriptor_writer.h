#pragma once

#include "gpu_resources.h"

#include <cstdint>
#include <vector>

// One reflected shader resource to write into an already-created Vulkan descriptor set.
// resourceId is a buffer object for buffer descriptors, or an SRV/UAV view object for images.
// samplerId is used by SAMPLER and COMBINED_IMAGE_SAMPLER descriptors.
struct PGAndroidDescriptorBinding {
    uint32_t binding = 0;
    uint32_t arrayElement = 0;
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
    uint32_t resourceId = 0;
    uint32_t samplerId = 0;
    VkDeviceSize offset = 0;
    VkDeviceSize range = VK_WHOLE_SIZE;
};

// Resolves the Android resource table into Vulkan writes. Descriptor-set layouts and
// pipeline layouts remain owned by the shader/pipeline stage.
class PGAndroidDescriptorWriter {
public:
    bool createSetLayout(VkDevice device, const PGAndroidDescriptorBinding *bindings,
                         uint32_t count, VkDescriptorSetLayout &out) const;
    bool update(VkDevice device, VkDescriptorSet set, const PGAndroidGpuResources &resources,
                const PGAndroidDescriptorBinding *bindings, uint32_t count) const;
};
