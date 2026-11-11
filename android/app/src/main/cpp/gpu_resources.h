#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <unordered_map>

class PGAndroidGpuResources {
public:
    explicit PGAndroidGpuResources(VkPhysicalDevice physical, VkDevice device);
    ~PGAndroidGpuResources();
    PGAndroidGpuResources(const PGAndroidGpuResources &) = delete;
    PGAndroidGpuResources &operator=(const PGAndroidGpuResources &) = delete;

    bool createBuffer(const uint32_t *words, uint32_t wordCount);
    bool createTexture(const uint32_t *words, uint32_t wordCount);
    bool uploadBuffer(const uint32_t *words, uint32_t wordCount);
    bool destroyObject(uint32_t objectId);
    void reset();

private:
    struct TextureResource {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };
    struct BufferResource {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void *mapped = nullptr;
        VkDeviceSize size = 0;
        VkMemoryPropertyFlags memoryFlags = 0;
    };
    uint32_t findMemoryType(uint32_t bits, VkMemoryPropertyFlags required,
                            VkMemoryPropertyFlags preferred) const;
    VkPhysicalDevice physical_;
    VkDevice device_;
    std::unordered_map<uint32_t, BufferResource> buffers_;
    std::unordered_map<uint32_t, TextureResource> textures_;
};
