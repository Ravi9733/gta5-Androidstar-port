#include "gpu_descriptor_writer.h"

#include <unordered_set>

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
                if (!resources.getBufferDescriptor(binding.resourceId, binding.offset, binding.range,
                                                     info, 0x08u | 0x80u)) return false;
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
