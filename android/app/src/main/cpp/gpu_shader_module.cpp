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
    if (!PGAndroidBuildShaderDescriptorBindings(stageIndex, resources, resourceCount,
                                                 state, descriptors)) return false;
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize=byteCount;
    info.pCode=spirv;
    VkShaderModule module=VK_NULL_HANDLE;
    if (vkCreateShaderModule(device,&info,nullptr,&module)!=VK_SUCCESS) return false;
    out.module=module;
    out.stage=stage;
    out.descriptors=std::move(descriptors);
    return true;
}

void PGAndroidDestroyShaderModule(VkDevice device, PGAndroidShaderModule &shader) {
    if (device && shader.module) vkDestroyShaderModule(device,shader.module,nullptr);
    shader.module=VK_NULL_HANDLE;
    shader.stage=VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    shader.descriptors.clear();
}
