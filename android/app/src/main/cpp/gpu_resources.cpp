#include "gpu_resources.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace {
constexpr uint32_t kBindVertex = 0x01;
constexpr uint32_t kBindIndex = 0x02;
constexpr uint32_t kBindConstant = 0x04;
constexpr uint32_t kBindShaderResource = 0x08;
constexpr uint32_t kBindUnorderedAccess = 0x80;
constexpr uint32_t kMaxBufferBytes = 512u * 1024u * 1024u;
}


namespace {
VkFormat dxgiToVkFormat(uint32_t dxgi) {
    switch (dxgi) {
        case 2: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case 10: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case 16: return VK_FORMAT_R32G32_SFLOAT;
        case 28: return VK_FORMAT_R8G8B8A8_UNORM;
        case 29: return VK_FORMAT_R8G8B8A8_SRGB;
        case 34: return VK_FORMAT_R16G16_SFLOAT;
        case 40: case 41: return VK_FORMAT_R32_SFLOAT;
        case 45: return VK_FORMAT_D24_UNORM_S8_UINT;
        case 49: return VK_FORMAT_R8G8_UNORM;
        case 54: return VK_FORMAT_R16_SFLOAT;
        case 55: return VK_FORMAT_D16_UNORM;
        case 61: case 65: return VK_FORMAT_R8_UNORM;
        case 71: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case 72: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case 74: return VK_FORMAT_BC2_UNORM_BLOCK;
        case 75: return VK_FORMAT_BC2_SRGB_BLOCK;
        case 77: return VK_FORMAT_BC3_UNORM_BLOCK;
        case 78: return VK_FORMAT_BC3_SRGB_BLOCK;
        case 80: return VK_FORMAT_BC4_UNORM_BLOCK;
        case 81: return VK_FORMAT_BC4_SNORM_BLOCK;
        case 83: return VK_FORMAT_BC5_UNORM_BLOCK;
        case 84: return VK_FORMAT_BC5_SNORM_BLOCK;
        case 87: return VK_FORMAT_B8G8R8A8_UNORM;
        case 91: return VK_FORMAT_B8G8R8A8_SRGB;
        case 95: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case 96: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case 98: return VK_FORMAT_BC7_UNORM_BLOCK;
        case 99: return VK_FORMAT_BC7_SRGB_BLOCK;
        default: return VK_FORMAT_UNDEFINED;
    }
}
bool isDepthVkFormat(VkFormat format) {
    return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}
}

PGAndroidGpuResources::PGAndroidGpuResources(VkPhysicalDevice physical, VkDevice device)
    : physical_(physical), device_(device) {}

PGAndroidGpuResources::~PGAndroidGpuResources() { reset(); }

uint32_t PGAndroidGpuResources::findMemoryType(
    uint32_t bits, VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred) const {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical_, &properties);
    uint32_t fallback = UINT32_MAX;
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if (!(bits & (1u << i))) continue;
        const auto flags = properties.memoryTypes[i].propertyFlags;
        if ((flags & required) != required) continue;
        if ((flags & preferred) == preferred) return i;
        if (fallback == UINT32_MAX) fallback = i;
    }
    return fallback;
}

bool PGAndroidGpuResources::createBuffer(const uint32_t *p, uint32_t n) {
    if (!p || n < 5) return false;
    const uint32_t id = p[0], bytes = p[1], bind = p[2];
    if (!id || !bytes || bytes > kMaxBufferBytes || buffers_.count(id)) return false;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (bind & kBindVertex) usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (bind & kBindIndex) usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (bind & kBindConstant) usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (bind & (kBindShaderResource | kBindUnorderedAccess))
        usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    BufferResource resource{};
    resource.size = bytes;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = bytes;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &info, nullptr, &resource.buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, resource.buffer, &requirements);
    const uint32_t memoryType = findMemoryType(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == UINT32_MAX) {
        vkDestroyBuffer(device_, resource.buffer, nullptr);
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physical_, &memoryProperties);
    resource.memoryFlags = memoryProperties.memoryTypes[memoryType].propertyFlags;
    if (vkAllocateMemory(device_, &allocation, nullptr, &resource.memory) != VK_SUCCESS ||
        vkBindBufferMemory(device_, resource.buffer, resource.memory, 0) != VK_SUCCESS ||
        vkMapMemory(device_, resource.memory, 0, VK_WHOLE_SIZE, 0, &resource.mapped) != VK_SUCCESS) {
        vkDestroyBuffer(device_, resource.buffer, nullptr);
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
        return false;
    }
    std::memset(resource.mapped, 0, bytes);
    buffers_.emplace(id, resource);
    return true;
}


bool PGAndroidGpuResources::createTexture(const uint32_t *p, uint32_t n) {
    if (!p || n < 10) return false;
    const uint32_t id=p[0], dim=p[1], width=p[2], height=p[3], depth=p[4];
    const uint32_t mipLevels=p[5], layers=p[6], dxgi=p[7], bind=p[8], samples=p[9];
    if (!id || !width || !height || !depth || !mipLevels || !layers ||
        dim < 1 || dim > 3 || samples == 0 || (samples & (samples-1)) ||
        textures_.count(id) || buffers_.count(id)) return false;
    const uint32_t maxDimension = std::max(width, std::max(height, depth));
    uint32_t maxMips=1;
    for (uint32_t d=maxDimension; d>1; d>>=1) ++maxMips;
    if (mipLevels > maxMips || (dim == 1 && height != 1) ||
        (dim == 3 && layers != 1) || (samples > 1 && (mipLevels != 1 || dim != 2)))
        return false;

    const VkFormat format=dxgiToVkFormat(dxgi);
    if (format == VK_FORMAT_UNDEFINED) return false;
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(physical_, format, &formatProperties);
    VkImageUsageFlags usage=VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (bind & kBindShaderResource) usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (bind & 0x20u) usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (bind & 0x40u) usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (bind & kBindUnorderedAccess) usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    VkFormatFeatureFlags required=0;
    if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) required |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) required |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) required |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (usage & VK_IMAGE_USAGE_STORAGE_BIT) required |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    if ((formatProperties.optimalTilingFeatures & required) != required) return false;

    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = dim == 1 ? VK_IMAGE_TYPE_1D : dim == 3 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    info.format=format;
    info.extent={width, dim == 1 ? 1u : height, dim == 3 ? depth : 1u};
    info.mipLevels=mipLevels;
    info.arrayLayers=dim == 3 ? 1u : layers;
    info.samples=static_cast<VkSampleCountFlagBits>(samples);
    info.tiling=VK_IMAGE_TILING_OPTIMAL;
    info.usage=usage;
    info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;

    TextureResource resource{};
    if (vkCreateImage(device_, &info, nullptr, &resource.image) != VK_SUCCESS) return false;
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, resource.image, &requirements);
    const uint32_t memoryType=findMemoryType(requirements.memoryTypeBits, 0,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryType == UINT32_MAX) {
        vkDestroyImage(device_, resource.image, nullptr);
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize=requirements.size;
    allocation.memoryTypeIndex=memoryType;
    if (vkAllocateMemory(device_, &allocation, nullptr, &resource.memory) != VK_SUCCESS ||
        vkBindImageMemory(device_, resource.image, resource.memory, 0) != VK_SUCCESS) {
        vkDestroyImage(device_, resource.image, nullptr);
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
        return false;
    }
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image=resource.image;
    viewInfo.viewType=dim == 1 ? (layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D) :
                      dim == 3 ? VK_IMAGE_VIEW_TYPE_3D :
                      layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format=format;
    viewInfo.subresourceRange.aspectMask=isDepthVkFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    viewInfo.subresourceRange.baseMipLevel=0;
    viewInfo.subresourceRange.levelCount=mipLevels;
    viewInfo.subresourceRange.baseArrayLayer=0;
    viewInfo.subresourceRange.layerCount=dim == 3 ? 1u : layers;
    if (vkCreateImageView(device_, &viewInfo, nullptr, &resource.view) != VK_SUCCESS) {
        vkDestroyImage(device_, resource.image, nullptr);
        vkFreeMemory(device_, resource.memory, nullptr);
        return false;
    }
    textures_.emplace(id, resource);
    return true;
}

bool PGAndroidGpuResources::uploadBuffer(const uint32_t *p, uint32_t n) {
    if (!p || n < 3) return false;
    const uint32_t id = p[0], offset = p[1], bytes = p[2];
    auto it = buffers_.find(id);
    if (it == buffers_.end() || offset > it->second.size ||
        bytes > it->second.size - offset || static_cast<uint64_t>(bytes) > static_cast<uint64_t>(n - 3u) * sizeof(uint32_t))
        return false;
    if (bytes) {
        std::memcpy(static_cast<uint8_t *>(it->second.mapped) + offset, p + 3, bytes);
        if (!(it->second.memoryFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
            range.memory = it->second.memory;
            range.offset = 0;
            range.size = VK_WHOLE_SIZE;
            if (vkFlushMappedMemoryRanges(device_, 1, &range) != VK_SUCCESS) return false;
        }
    }
    return true;
}

bool PGAndroidGpuResources::destroyObject(uint32_t id) {
    auto it = buffers_.find(id);
    if (it == buffers_.end()) {
        auto texture = textures_.find(id);
        if (texture == textures_.end()) return false;
        if (texture->second.view) vkDestroyImageView(device_, texture->second.view, nullptr);
        if (texture->second.image) vkDestroyImage(device_, texture->second.image, nullptr);
        if (texture->second.memory) vkFreeMemory(device_, texture->second.memory, nullptr);
        textures_.erase(texture);
        return true;
    }
    if (it->second.mapped) vkUnmapMemory(device_, it->second.memory);
    vkDestroyBuffer(device_, it->second.buffer, nullptr);
    vkFreeMemory(device_, it->second.memory, nullptr);
    buffers_.erase(it);
    return true;
}

void PGAndroidGpuResources::reset() {
    for (auto &entry : buffers_) {
        auto &resource = entry.second;
        if (resource.mapped) vkUnmapMemory(device_, resource.memory);
        if (resource.buffer) vkDestroyBuffer(device_, resource.buffer, nullptr);
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
    }
    buffers_.clear();
    for (auto &entry : textures_) {
        auto &resource=entry.second;
        if (resource.view) vkDestroyImageView(device_, resource.view, nullptr);
        if (resource.image) vkDestroyImage(device_, resource.image, nullptr);
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
    }
    textures_.clear();
}
