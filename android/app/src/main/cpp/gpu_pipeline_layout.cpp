#include "gpu_pipeline_layout.h"
#include <algorithm>
#include <unordered_map>
#include <utility>

namespace {
bool sameBinding(const PGAndroidDescriptorBinding &a,const PGAndroidDescriptorBinding &b) {
    return a.type==b.type && a.arrayElement==b.arrayElement;
}
}
bool PGAndroidCreatePipelineLayout(VkDevice device, PGAndroidDescriptorPool &pool,
    const PGAndroidShaderModule *shaders,uint32_t shaderCount,
    PGAndroidDescriptorWriter &writer,PGAndroidPipelineLayout &out) {
    if (!device || !shaders || !shaderCount || out.pipelineLayout) return false;
    std::array<std::unordered_map<uint32_t,PGAndroidDescriptorBinding>,3> merged;
    for(uint32_t s=0;s<shaderCount;++s) {
        if(!shaders[s].module || shaders[s].stageIndex>=3) return false;
        auto &stage=merged[shaders[s].stageIndex];
        for(const auto &b:shaders[s].descriptors) {
            auto it=stage.find(b.binding);
            if(it==stage.end()) stage.emplace(b.binding,b);
            else if(!sameBinding(it->second,b)) return false;
            else it->second.stageFlags|=b.stageFlags;
        }
    }
    PGAndroidPipelineLayout result{};
    for(uint32_t stage=0;stage<3;++stage) {
        auto &bindings=result.bindings[stage];
        for(const auto &entry:merged[stage]) bindings.push_back(entry.second);
        std::sort(bindings.begin(),bindings.end(),[](const auto&a,const auto&b){return a.binding<b.binding;});
        if(!writer.createSetLayout(device,bindings.data(),static_cast<uint32_t>(bindings.size()),result.setLayouts[stage])) goto fail;
        if(!pool.allocate(result.setLayouts[stage],result.descriptorSets[stage])) goto fail;
    }
    {
        VkPipelineLayoutCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        info.setLayoutCount=3; info.pSetLayouts=result.setLayouts.data();
        if(vkCreatePipelineLayout(device,&info,nullptr,&result.pipelineLayout)!=VK_SUCCESS) goto fail;
    }
    out=std::move(result);
    return true;
fail:
    for(uint32_t i=0;i<3;++i) {
        if(result.descriptorSets[i]) (void)pool.free(result.descriptorSets[i]);
        if(result.setLayouts[i]) vkDestroyDescriptorSetLayout(device,result.setLayouts[i],nullptr);
    }
    if(result.pipelineLayout) vkDestroyPipelineLayout(device,result.pipelineLayout,nullptr);
    return false;
}

bool PGAndroidUpdatePipelineDescriptors(VkDevice device,const PGAndroidPipelineLayout &layout,
    const PGAndroidShaderModule *shaders,uint32_t shaderCount,
    const PGAndroidGpuBindingState &state,const PGAndroidGpuResources &resources,
    PGAndroidDescriptorWriter &writer) {
    if(!device || !shaders || !shaderCount || !layout.pipelineLayout) return false;
    std::array<std::unordered_map<uint32_t,PGAndroidDescriptorBinding>,3> resolved;
    for(uint32_t i=0;i<shaderCount;++i) {
        if(shaders[i].stageIndex>=3) return false;
        std::vector<PGAndroidDescriptorBinding> stageBindings;
        if(!PGAndroidRefreshShaderDescriptors(shaders[i],state,stageBindings)) return false;
        auto &stage=resolved[shaders[i].stageIndex];
        for(const auto &b:stageBindings) {
            auto it=stage.find(b.binding);
            if(it==stage.end()) resolved[shaders[i].stageIndex].emplace(b.binding,b);
            else {
                const auto &p=it->second;
                if(!sameBinding(p,b) || p.resourceId!=b.resourceId || p.samplerId!=b.samplerId ||
                   p.offset!=b.offset || p.range!=b.range) return false;
            }
        }
    }
    for(uint32_t stage=0;stage<3;++stage) {
        std::vector<PGAndroidDescriptorBinding> writes;
        for(const auto &entry:resolved[stage]) writes.push_back(entry.second);
        std::sort(writes.begin(),writes.end(),[](const auto&a,const auto&b){return a.binding<b.binding;});
        if(writes.size()!=layout.bindings[stage].size()) return false;
        for(size_t i=0;i<writes.size();++i)
            if(writes[i].binding!=layout.bindings[stage][i].binding ||
               writes[i].type!=layout.bindings[stage][i].type) return false;
        if(!writer.update(device,layout.descriptorSets[stage],resources,writes.data(),
                          static_cast<uint32_t>(writes.size()))) return false;
    }
    return true;
}
bool PGAndroidPreparePipelineDescriptors(VkCommandBuffer commandBuffer,VkDevice device,
    const PGAndroidPipelineLayout &layout,const PGAndroidShaderModule *shaders,
    uint32_t shaderCount,const PGAndroidGpuBindingState &state,
    PGAndroidGpuResources &resources,PGAndroidDescriptorWriter &writer) {
    if(!commandBuffer || !device || !layout.pipelineLayout || !shaders || !shaderCount)
        return false;
    bool hasStorageBuffer=false;
    for(uint32_t i=0;i<shaderCount;++i) {
        std::vector<PGAndroidDescriptorBinding> live;
        if(!PGAndroidRefreshShaderDescriptors(shaders[i],state,live)) return false;
        for(const auto &binding:live) {
            if(binding.type==VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
                if(!binding.resourceId ||
                   !resources.prepareShaderResource(commandBuffer,binding.resourceId,false))
                    return false;
            } else if(binding.type==VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                if(!binding.resourceId ||
                   !resources.prepareShaderResource(commandBuffer,binding.resourceId,true))
                    return false;
            } else if(binding.type==VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                hasStorageBuffer=true;
            }
        }
    }
    if(hasStorageBuffer) {
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT|
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,nullptr,0,nullptr);
    }
    return PGAndroidUpdatePipelineDescriptors(device,layout,shaders,shaderCount,
        state,resources,writer);
}

void PGAndroidDestroyPipelineLayout(VkDevice device,PGAndroidDescriptorPool &pool,
    PGAndroidPipelineLayout &layout) {
    if(device && layout.pipelineLayout) vkDestroyPipelineLayout(device,layout.pipelineLayout,nullptr);
    for(uint32_t i=0;i<3;++i) {
        if(device && layout.descriptorSets[i]) (void)pool.free(layout.descriptorSets[i]);
        if(device && layout.setLayouts[i]) vkDestroyDescriptorSetLayout(device,layout.setLayouts[i],nullptr);
        layout.descriptorSets[i]=VK_NULL_HANDLE; layout.setLayouts[i]=VK_NULL_HANDLE;
        layout.bindings[i].clear();
    }
    layout.pipelineLayout=VK_NULL_HANDLE;
}
