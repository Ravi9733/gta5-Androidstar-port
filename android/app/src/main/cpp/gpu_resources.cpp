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
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
        vkDestroyBuffer(device_, resource.buffer, nullptr);
        return false;
    }
    std::memset(resource.mapped, 0, bytes);
    buffers_.emplace(id, resource);
    return true;
}

bool PGAndroidGpuResources::uploadBuffer(const uint32_t *p, uint32_t n) {
    if (!p || n < 3) return false;
    const uint32_t id = p[0], offset = p[1], bytes = p[2];
    auto it = buffers_.find(id);
    if (it == buffers_.end() || offset > it->second.size ||
        bytes > it->second.size - offset || bytes > (n - 3u) * sizeof(uint32_t))
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
    if (it == buffers_.end()) return false;
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
}
