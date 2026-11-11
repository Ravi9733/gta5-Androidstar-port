#include "gpu_descriptor_writer.h"

#include <unordered_set>

#include <algorithm>
#include <unordered_map>

bool PGAndroidDescriptorWriter::createSetLayout(
    VkDevice device, const PGAndroidDescriptorBinding *bindings, uint32_t count,
    VkDescriptorSetLayout &out) const {
    if (device == VK_NULL_HANDLE || (!bindings && count) || out != VK_NULL_HANDLE) return false;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> byBinding;
    std::unordered_map<uint32_t, uint32_t> maxArrayElement;
    for (uint32_t i = 0; i < count; ++i) {
        const auto &source = bindings[i];
        if (source.type == VK_DESCRIPTOR_TYPE_MAX_ENUM || source.stageFlags == 0) return false;
        auto it = byBinding.find(source.binding);
        if (it == byBinding.end()) {
            VkDescriptorSetLayoutBinding entry{};
            entry.binding = source.binding;
            entry.descriptorType = source.type;
            entry.descriptorCount = 1;
            entry.stageFlags = source.stageFlags;
            byBinding.emplace(source.binding, entry);
            maxArrayElement[source.binding] = source.arrayElement;
        } else {
            if (it->second.descriptorType != source.type) return false;
            it->second.stageFlags |= source.stageFlags;
            maxArrayElement[source.binding] = std::max(maxArrayElement[source.binding], source.arrayElement);
        }
    }
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    layoutBindings.reserve(byBinding.size());
    for (auto &entry : byBinding) {
        entry.second.descriptorCount = maxArrayElement[entry.first] + 1;
        layoutBindings.push_back(entry.second);
    }
    std::sort(layoutBindings.begin(), layoutBindings.end(),
              [](const auto &a, const auto &b) { return a.binding < b.binding; });
    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    info.pBindings = layoutBindings.data();
    return vkCreateDescriptorSetLayout(device, &info, nullptr, &out) == VK_SUCCESS;
}

bool PGAndroidDescriptorWriter::update(
    VkDevice device, VkDescriptorSet set, const PGAndroidGpuResources &resources,
    const PGAndroidDescriptorBinding *bindings, uint32_t count) const {
    if (device == VK_NULL_HANDLE || set == VK_NULL_HANDLE || (!bindings && count)) return false;
    if (count == 0) return true;

    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::unordered_set<uint64_t> occupied;
    writes.reserve(count);
    bufferInfos.reserve(count);
    imageInfos.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        const auto &binding = bindings[i];
        const uint64_t key = (static_cast<uint64_t>(binding.binding) << 32) | binding.arrayElement;
        if (!occupied.insert(key).second) return false;

        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = set;
        write.dstBinding = binding.binding;
        write.dstArrayElement = binding.arrayElement;
        write.descriptorCount = 1;
        write.descriptorType = binding.type;

        switch (binding.type) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: {
                VkDescriptorBufferInfo info{};
                if (!resources.getBufferDescriptor(binding.resourceId, binding.offset, binding.range,
                                                     info, 0x04u)) return false;
                bufferInfos.push_back(info);
                write.pBufferInfo = &bufferInfos.back();
                break;
            }
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
                VkDescriptorBufferInfo info{};
                // A reflected storage buffer may be read-only (SRV) or writable (UAV).
                // Resolve either legal D3D bind capability without requiring both flags.
                if (!resources.getBufferDescriptor(binding.resourceId, binding.offset, binding.range,
                                                    info, 0x08u) &&
                    !resources.getBufferDescriptor(binding.resourceId, binding.offset, binding.range,
                                                    info, 0x80u)) return false;
                bufferInfos.push_back(info);
                write.pBufferInfo = &bufferInfos.back();
                break;
            }
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                VkDescriptorImageInfo info{};
                if (binding.type == VK_DESCRIPTOR_TYPE_SAMPLER) {
                    if (!resources.getSamplerDescriptor(binding.samplerId, info)) return false;
                } else {
                    const bool uav = binding.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    if (!resources.getImageDescriptor(binding.resourceId, info, uav)) return false;
                    if (binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                        VkDescriptorImageInfo sampler{};
                        if (!resources.getSamplerDescriptor(binding.samplerId, sampler)) return false;
                        info.sampler = sampler.sampler;
                    }
                }
                imageInfos.push_back(info);
                write.pImageInfo = &imageInfos.back();
                break;
            }
            default:
                return false;
        }
        writes.push_back(write);
    }

    // All info vectors were reserved to count, so the pointers stored in writes remain stable.
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return true;
}
