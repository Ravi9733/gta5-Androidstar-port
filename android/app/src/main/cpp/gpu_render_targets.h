#pragma once

#include "gpu_resources.h"
#include <vulkan/vulkan.h>
#include <cstdint>

struct PGAndroidRenderTargets {
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    uint32_t width = 0, height = 0;
    uint32_t colorCount = 0;
    uint32_t colorViewIds[8]{};
    uint32_t depthViewId = 0;
    bool hasDepth = false;
};

bool PGAndroidCreateRenderTargets(VkDevice device, const PGAndroidGpuResources &resources,
    const uint32_t *colorViewIds, uint32_t colorCount, uint32_t depthViewId,
    uint32_t width, uint32_t height, PGAndroidRenderTargets &out);
bool PGAndroidBeginRenderTargets(VkCommandBuffer commandBuffer,
    PGAndroidGpuResources &resources, const uint32_t *colorViewIds,
    uint32_t colorCount, uint32_t depthViewId, const PGAndroidRenderTargets &targets,
    const VkClearColorValue *colorClears, float depthClear, uint32_t stencilClear);
void PGAndroidDestroyRenderTargets(VkDevice device, PGAndroidRenderTargets &targets);
