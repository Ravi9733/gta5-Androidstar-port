#include "gpu_shader_module.h"

#include <utility>

bool PGAndroidCreateShaderModule(
    VkDevice device, const uint32_t *spirv, size_t byteCount, VkShaderStageFlagBits stage,
    const PGAndroidShaderResource *resources, uint32_t resourceCount,
    const PGAndroidGpuBindingState &state, PGAndroidShaderModule &out) {
    if (device == VK_NULL_HANDLE || !spirv || byteCount < 20 || (byteCount & 3u) ||
        out.module != VK_NULL_HANDLE) return false;
    if (spirv[0] != 0x07230203u || spirv[3] == 0 || spirv[4] != 0) return false;
    uint32_t stageIndex;
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT: stageIndex=0; break;
        case VK_SHADER_STAGE_FRAGMENT_BIT: stageIndex=1; break;
        case VK_SHADER_STAGE_COMPUTE_BIT: stageIndex=2; break;
        default: return false;
    }
    std::vector<PGAndroidDescriptorBinding> descriptors;
    descriptors.reserve(resourceCount);
    const VkShaderStageFlags stageFlags=stage;
    for (uint32_t i=0;i<resourceCount;++i) {
        const auto &resource=resources[i];
        PGAndroidDescriptorBinding binding{};
        binding.binding=resource.binding;
        binding.stageFlags=stageFlags;
        switch (resource.kind) {
            case PG_ANDROID_RES_UNIFORM_BUFFER:
                if (resource.binding >= 15) return false;
                binding.type=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case PG_ANDROID_RES_STORAGE_READ_ONLY:
            case PG_ANDROID_RES_STORAGE_READ_WRITE:
                if (resource.binding < 32 || resource.binding >= 176) return false;
                binding.type=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            case PG_ANDROID_RES_SAMPLER:
                if (resource.binding < 16 || resource.binding >= 32) return false;
                binding.type=VK_DESCRIPTOR_TYPE_SAMPLER;
                break;
            case PG_ANDROID_RES_TEXTURE:
            case PG_ANDROID_RES_TEXTURE_MS:
            case PG_ANDROID_RES_DEPTH_TEXTURE:
            case PG_ANDROID_RES_DEPTH_TEXTURE_MS:
                if (resource.binding < 32 || resource.binding >= 160) return false;
                binding.type=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                break;
            case PG_ANDROID_RES_STORAGE_TEXTURE:
                if (resource.binding < 160 || resource.binding >= 176) return false;
                binding.type=VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                break;
            default: return false;
        }
        descriptors.push_back(binding);
    }
    (void)state; // Resources are resolved from live SET_* state immediately before a draw/dispatch.
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize=byteCount;
    info.pCode=spirv;
    VkShaderModule module=VK_NULL_HANDLE;
    if (vkCreateShaderModule(device,&info,nullptr,&module)!=VK_SUCCESS) return false;
    out.module=module;
    out.stage=stage;
    out.stageIndex=stageIndex;
    out.reflection.assign(resources,resources+resourceCount);
    out.descriptors=std::move(descriptors);
    return true;
}


bool PGAndroidRefreshShaderDescriptors(const PGAndroidShaderModule &shader,
    const PGAndroidGpuBindingState &state, std::vector<PGAndroidDescriptorBinding> &out) {
    if (!shader.module || shader.stageIndex >= 3) return false;
    return PGAndroidBuildShaderDescriptorBindings(shader.stageIndex,
        shader.reflection.data(), static_cast<uint32_t>(shader.reflection.size()), state, out);
}

void PGAndroidDestroyShaderModule(VkDevice device, PGAndroidShaderModule &shader) {
    if (device && shader.module) vkDestroyShaderModule(device,shader.module,nullptr);
    shader.module=VK_NULL_HANDLE;
    shader.stage=VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    shader.stageIndex=0;
    shader.reflection.clear();
    shader.descriptors.clear();
}
