#pragma once

#include "gpu_binding_state.h"
#include "gpu_descriptor_writer.h"

#include <cstdint>
#include <vector>

// Values mirror ResKind in app/Source/MetalRenderer/MRInternal.h.
enum PGAndroidShaderResourceKind : uint32_t {
    PG_ANDROID_RES_UNIFORM_BUFFER = 0,
    PG_ANDROID_RES_STORAGE_READ_ONLY = 1,
    PG_ANDROID_RES_STORAGE_READ_WRITE = 2,
    PG_ANDROID_RES_SAMPLER = 3,
    PG_ANDROID_RES_TEXTURE = 4,
    PG_ANDROID_RES_TEXTURE_MS = 5,
    PG_ANDROID_RES_DEPTH_TEXTURE = 6,
    PG_ANDROID_RES_DEPTH_TEXTURE_MS = 7,
    PG_ANDROID_RES_STORAGE_TEXTURE = 8
};

struct PGAndroidShaderResource {
    uint32_t kind = 0;
    uint32_t binding = 0; // Original WGSL/D3D register binding, not the Metal argument index.
    uint32_t size = 0;    // Uniform-buffer byte size from shader reflection.
};

// Resolve one original iOS ShaderInfo::res list against SET_* state into Vulkan
// descriptor writes. Unsupported special counter/constant-table bindings fail closed.
bool PGAndroidBuildShaderDescriptorBindings(
    uint32_t stage, const PGAndroidShaderResource *resources, uint32_t resourceCount,
    const PGAndroidGpuBindingState &state, std::vector<PGAndroidDescriptorBinding> &out);
