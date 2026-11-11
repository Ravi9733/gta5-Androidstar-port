#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

class PGAndroidDescriptorPool {
public:
    PGAndroidDescriptorPool() = default;
    ~PGAndroidDescriptorPool();
    PGAndroidDescriptorPool(const PGAndroidDescriptorPool &) = delete;
    PGAndroidDescriptorPool &operator=(const PGAndroidDescriptorPool &) = delete;

    bool initialize(VkDevice device, uint32_t maxSets = 64, uint32_t descriptorsPerType = 256);
    bool allocate(VkDescriptorSetLayout layout, VkDescriptorSet &out);
    bool free(VkDescriptorSet set);
    bool reset();
    void destroy();
    VkDescriptorPool handle() const { return pool_; }
    uint32_t allocatedCount() const { return allocatedCount_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    uint32_t maxSets_ = 0;
    uint32_t allocatedCount_ = 0;
};
