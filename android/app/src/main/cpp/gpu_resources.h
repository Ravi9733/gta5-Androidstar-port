#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <unordered_map>

class PGAndroidGpuResources {
public:
    PGAndroidGpuResources(VkPhysicalDevice physical, VkDevice device, VkQueue queue, uint32_t queueFamily, bool textureCompressionBC);
    ~PGAndroidGpuResources();
    PGAndroidGpuResources(const PGAndroidGpuResources &) = delete;
    PGAndroidGpuResources &operator=(const PGAndroidGpuResources &) = delete;

    bool createBuffer(const uint32_t *words, uint32_t wordCount);
    bool createTexture(const uint32_t *words, uint32_t wordCount);
    bool uploadBuffer(const uint32_t *words, uint32_t wordCount);
    bool uploadTexture(const uint32_t *words, uint32_t wordCount);
    bool createTextureView(const uint32_t *words, uint32_t wordCount, bool unorderedAccess);
    bool createSamplerState(const uint32_t *words, uint32_t wordCount);
    bool getBufferHandle(uint32_t objectId, uint32_t requiredBind, VkBuffer &out, VkDeviceSize &size) const;
    bool getBufferDescriptor(uint32_t objectId, VkDeviceSize offset, VkDeviceSize range,
                             VkDescriptorBufferInfo &out, uint32_t requiredBind) const;
    bool getAttachmentInfo(uint32_t viewId, bool depth, VkImageView &view, VkFormat &format,
                          VkSampleCountFlagBits &samples) const;
    bool prepareAttachment(VkCommandBuffer commandBuffer, uint32_t viewId, bool depth,
                           VkImageView &view, VkFormat &format, VkSampleCountFlagBits &samples);
    bool getImageDescriptor(uint32_t viewId, VkDescriptorImageInfo &out,
                            bool unorderedAccess) const;
    bool getBufferViewDescriptor(uint32_t viewId, VkDescriptorBufferInfo &out,
                                 bool unorderedAccess) const;
    bool getSamplerDescriptor(uint32_t samplerId, VkDescriptorImageInfo &out) const;
    bool destroyObject(uint32_t objectId);
    void reset();

private:
    struct TextureResource {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint32_t width = 0, height = 0, depth = 0, mipLevels = 0, layers = 0;
        uint32_t dimension = 0, bind = 0, bcFormat = 0, sourceFormat = 0;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        bool decodeBC = false;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };
    struct ViewResource {
        VkImageView view = VK_NULL_HANDLE;
        uint32_t resourceId = 0;
        uint32_t firstElement = 0;
        bool unorderedAccess = false;
        bool bufferView = false;
    };
    struct BufferResource {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void *mapped = nullptr;
        VkDeviceSize size = 0;
        VkMemoryPropertyFlags memoryFlags = 0;
        uint32_t bind = 0;
        uint32_t stride = 0;
    };
    uint32_t findMemoryType(uint32_t bits, VkMemoryPropertyFlags required,
                            VkMemoryPropertyFlags preferred) const;
    VkPhysicalDevice physical_;
    VkDevice device_;
    VkQueue queue_;
    uint32_t queueFamily_;
    bool textureCompressionBC_;
    std::unordered_map<uint32_t, BufferResource> buffers_;
    std::unordered_map<uint32_t, TextureResource> textures_;
    std::unordered_map<uint32_t, ViewResource> views_;
    std::unordered_map<uint32_t, VkSampler> samplers_;
};
