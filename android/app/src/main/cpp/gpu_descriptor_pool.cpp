#include "gpu_descriptor_pool.h"

#include <array>
#include <limits>

PGAndroidDescriptorPool::~PGAndroidDescriptorPool() { destroy(); }

bool PGAndroidDescriptorPool::initialize(VkDevice device, uint32_t maxSets,
                                          uint32_t descriptorsPerType) {
    if (device == VK_NULL_HANDLE || pool_ != VK_NULL_HANDLE || maxSets == 0 ||
        descriptorsPerType == 0 ||
        descriptorsPerType > std::numeric_limits<uint32_t>::max() / maxSets) return false;

    constexpr std::array<VkDescriptorType, 8> types = {
        VK_DESCRIPTOR_TYPE_SAMPLER,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
    };
    const uint32_t perType = maxSets * descriptorsPerType;
    std::array<VkDescriptorPoolSize, types.size()> sizes{};
    for (size_t i = 0; i < types.size(); ++i) {
        sizes[i].type = types[i];
        sizes[i].descriptorCount = perType;
    }

    VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.maxSets = maxSets;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes = sizes.data();
    VkDescriptorPool created = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(device, &info, nullptr, &created) != VK_SUCCESS) return false;
    device_ = device;
    pool_ = created;
    maxSets_ = maxSets;
    allocatedCount_ = 0;
    return true;
}

bool PGAndroidDescriptorPool::allocate(VkDescriptorSetLayout layout, VkDescriptorSet &out) {
    if (pool_ == VK_NULL_HANDLE || layout == VK_NULL_HANDLE ||
        allocatedCount_ >= maxSets_ || out != VK_NULL_HANDLE) return false;
    VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.descriptorPool = pool_;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;
    if (vkAllocateDescriptorSets(device_, &info, &out) != VK_SUCCESS) {
        out = VK_NULL_HANDLE;
        return false;
    }
    ++allocatedCount_;
    return true;
}

bool PGAndroidDescriptorPool::free(VkDescriptorSet set) {
    if (pool_ == VK_NULL_HANDLE || set == VK_NULL_HANDLE || allocatedCount_ == 0) return false;
    if (vkFreeDescriptorSets(device_, pool_, 1, &set) != VK_SUCCESS) return false;
    --allocatedCount_;
    return true;
}

bool PGAndroidDescriptorPool::reset() {
    if (pool_ == VK_NULL_HANDLE) return false;
    if (vkResetDescriptorPool(device_, pool_, 0) != VK_SUCCESS) return false;
    allocatedCount_ = 0;
    return true;
}

void PGAndroidDescriptorPool::destroy() {
    if (pool_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device_, pool_, nullptr);
    pool_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    maxSets_ = 0;
    allocatedCount_ = 0;
}
