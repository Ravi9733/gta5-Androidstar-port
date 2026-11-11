#include "gpu_shader_bindings.h"

bool PGAndroidBuildShaderDescriptorBindings(
    uint32_t stage, const PGAndroidShaderResource *resources, uint32_t resourceCount,
    const PGAndroidGpuBindingState &state, std::vector<PGAndroidDescriptorBinding> &out) {
    if (stage >= 3 || (!resources && resourceCount)) return false;
    const VkShaderStageFlags stageFlag = stage == 0 ? VK_SHADER_STAGE_VERTEX_BIT :
        stage == 1 ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_COMPUTE_BIT;
    std::vector<PGAndroidDescriptorBinding> resolved;
    resolved.reserve(resourceCount);

    for (uint32_t i = 0; i < resourceCount; ++i) {
        const auto &source = resources[i];
        PGAndroidDescriptorBinding binding{};
        binding.binding = source.binding;
        binding.arrayElement = 0;
        binding.stageFlags = stageFlag;

        switch (source.kind) {
            case PG_ANDROID_RES_UNIFORM_BUFFER: {
                // Binding 15 is Tint's generated constant table, not an engine CB slot.
                if (source.binding >= 15 || source.binding >= 16) return false;
                const auto &cb = state.constantBuffer(stage, source.binding);
                if (!cb.objectId) return false;
                binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                binding.resourceId = cb.objectId;
                binding.offset = static_cast<VkDeviceSize>(cb.offset) * 16u;
                binding.range = source.size ? source.size : VK_WHOLE_SIZE;
                break;
            }
            case PG_ANDROID_RES_STORAGE_READ_ONLY:
            case PG_ANDROID_RES_STORAGE_READ_WRITE: {
                // The iOS renderer routes t-registers through SRV views and u-registers
                // through UAV views. u176+ are hidden append/consume counters and need
                // a dedicated counter-buffer implementation, so reject them for now.
                if (source.binding < 32 || source.binding >= 176) return false;
                if (source.binding < 160) {
                    binding.resourceId = state.shaderResource(stage, (source.binding - 32u) & 127u);
                } else {
                    binding.resourceId = state.unorderedAccess(stage, (source.binding - 160u) & 7u).objectId;
                }
                if (!binding.resourceId) return false;
                binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            }
            case PG_ANDROID_RES_SAMPLER: {
                if (source.binding < 16 || source.binding >= 32) return false;
                binding.type = VK_DESCRIPTOR_TYPE_SAMPLER;
                binding.samplerId = state.sampler(stage, (source.binding - 16u) & 15u);
                if (!binding.samplerId) return false;
                break;
            }
            case PG_ANDROID_RES_TEXTURE:
            case PG_ANDROID_RES_TEXTURE_MS:
            case PG_ANDROID_RES_DEPTH_TEXTURE:
            case PG_ANDROID_RES_DEPTH_TEXTURE_MS: {
                if (source.binding < 32 || source.binding >= 160) return false;
                binding.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                binding.resourceId = state.shaderResource(stage, (source.binding - 32u) & 127u);
                if (!binding.resourceId) return false;
                break;
            }
            case PG_ANDROID_RES_STORAGE_TEXTURE: {
                if (source.binding < 160 || source.binding >= 176) return false;
                binding.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                binding.resourceId = state.unorderedAccess(stage, (source.binding - 160u) & 7u).objectId;
                if (!binding.resourceId) return false;
                break;
            }
            default:
                return false;
        }
        resolved.push_back(binding);
    }

    out = std::move(resolved);
    return true;
}
