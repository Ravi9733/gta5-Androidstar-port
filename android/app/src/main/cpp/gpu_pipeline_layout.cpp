#include "gpu_pipeline_layout.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

bool PGAndroidCreatePipelineLayout(
    VkDevice device, PGAndroidDescriptorPool &pool,
    const PGAndroidShaderModule *shaders, uint32_t shaderCount,
    PGAndroidDescriptorWriter &writer, PGAndroidPipelineLayout &out) {
    if (!device || !shaders || shaderCount == 0 || out.pipelineLayout ||
        out.setLayout || out.descriptorSet) return false;

    std::unordered_map<uint32_t, PGAndroidDescriptorBinding> merged;
    for (uint32_t s=0; s<shaderCount; ++s) {
        if (!shaders[s].module || shaders[s].stage == VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM) return false;
        for (const auto &source : shaders[s].descriptors) {
            auto it=merged.find(source.binding);
            if (it == merged.end()) {
                merged.emplace(source.binding,source);
            } else {
                if (it->second.type != source.type || it->second.arrayElement != source.arrayElement)
                    return false;
                it->second.stageFlags |= source.stageFlags;
            }
        }
    }

    std::vector<PGAndroidDescriptorBinding> bindings;
    bindings.reserve(merged.size());
    for (const auto &entry : merged) bindings.push_back(entry.second);
    std::sort(bindings.begin(),bindings.end(),[](const auto &a,const auto &b){return a.binding<b.binding;});

    VkDescriptorSetLayout setLayout=VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout=VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet=VK_NULL_HANDLE;
    if (!writer.createSetLayout(device,bindings.data(),static_cast<uint32_t>(bindings.size()),setLayout))
        return false;

    VkPipelineLayoutCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    info.setLayoutCount=1;
    info.pSetLayouts=&setLayout;
    if (vkCreatePipelineLayout(device,&info,nullptr,&pipelineLayout)!=VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device,setLayout,nullptr);
        return false;
    }
    if (!pool.allocate(setLayout,descriptorSet)) {
        vkDestroyPipelineLayout(device,pipelineLayout,nullptr);
        vkDestroyDescriptorSetLayout(device,setLayout,nullptr);
        return false;
    }
    out.setLayout=setLayout;
    out.pipelineLayout=pipelineLayout;
    out.descriptorSet=descriptorSet;
    out.bindings=std::move(bindings);
    return true;
}

bool PGAndroidUpdatePipelineDescriptors(
    VkDevice device, const PGAndroidPipelineLayout &layout, const PGAndroidShaderModule *shaders,
    uint32_t shaderCount, const PGAndroidGpuBindingState &state,
    const PGAndroidGpuResources &resources, PGAndroidDescriptorWriter &writer) {
    if (!device || !layout.descriptorSet || !shaders || shaderCount == 0) return false;
    std::unordered_map<uint32_t, PGAndroidDescriptorBinding> resolved;
    for (uint32_t i=0;i<shaderCount;++i) {
        std::vector<PGAndroidDescriptorBinding> stageBindings;
        if (!PGAndroidRefreshShaderDescriptors(shaders[i],state,stageBindings)) return false;
        for (const auto &binding : stageBindings) {
            auto it=resolved.find(binding.binding);
            if (it == resolved.end()) {
                resolved.emplace(binding.binding,binding);
                continue;
            }
            const auto &prior=it->second;
            if (prior.type != binding.type || prior.resourceId != binding.resourceId ||
                prior.samplerId != binding.samplerId || prior.offset != binding.offset ||
                prior.range != binding.range || prior.arrayElement != binding.arrayElement)
                return false; // Current single-set layout cannot represent stage-local resources.
            it->second.stageFlags |= binding.stageFlags;
        }
    }
    std::vector<PGAndroidDescriptorBinding> writes;
    writes.reserve(resolved.size());
    for (const auto &entry : resolved) writes.push_back(entry.second);
    std::sort(writes.begin(),writes.end(),[](const auto &a,const auto &b){return a.binding<b.binding;});
    if (writes.size() != layout.bindings.size()) return false;
    for (size_t i=0;i<writes.size();++i)
        if (writes[i].binding != layout.bindings[i].binding ||
            writes[i].type != layout.bindings[i].type) return false;
    return writer.update(device, layout.descriptorSet, resources,
                         writes.data(), static_cast<uint32_t>(writes.size()));
}

void PGAndroidDestroyPipelineLayout(VkDevice device, PGAndroidDescriptorPool &pool,
                                    PGAndroidPipelineLayout &layout) {
    if (device && layout.descriptorSet) (void)pool.free(layout.descriptorSet);
    if (device && layout.pipelineLayout) vkDestroyPipelineLayout(device,layout.pipelineLayout,nullptr);
    if (device && layout.setLayout) vkDestroyDescriptorSetLayout(device,layout.setLayout,nullptr);
    layout.descriptorSet=VK_NULL_HANDLE;
    layout.pipelineLayout=VK_NULL_HANDLE;
    layout.setLayout=VK_NULL_HANDLE;
    layout.bindings.clear();
}
