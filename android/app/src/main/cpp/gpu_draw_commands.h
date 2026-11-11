#pragma once

#include "gpu_pipeline.h"
#include "gpu_resources.h"
#include "gpu_render_targets.h"

#include <vulkan/vulkan.h>
#include <cstdint>

// Records one graphics draw. Descriptor contents must have been refreshed for the
// current D3D11 binding state before this call.
bool PGAndroidRecordDraw(VkCommandBuffer commandBuffer,
    const PGAndroidPipeline &pipeline, const PGAndroidPipelineLayout &layout,
    const PGAndroidGpuResources &resources,
    const uint32_t *vertexBufferIds, const VkDeviceSize *vertexOffsets,
    uint32_t vertexBufferCount, uint32_t indexBufferId, VkDeviceSize indexOffset,
    VkIndexType indexType, uint32_t vertexCount, uint32_t indexCount,
    uint32_t instanceCount, uint32_t firstVertexOrIndex, int32_t vertexOffset,
    uint32_t firstInstance);

// Records one compute dispatch using the pipeline's single descriptor set.
bool PGAndroidRecordDispatch(VkCommandBuffer commandBuffer,
    const PGAndroidPipeline &pipeline, const PGAndroidPipelineLayout &layout,
    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

/* Begins the selected render pass, records one draw, and always closes the pass. */
bool PGAndroidRecordDrawInRenderTargets(VkCommandBuffer commandBuffer,
    PGAndroidGpuResources &resources, const PGAndroidRenderTargets &targets,
    const uint32_t *colorViewIds, uint32_t colorCount, uint32_t depthViewId,
    const VkClearColorValue *colorClears, float depthClear, uint32_t stencilClear,
    const PGAndroidPipeline &pipeline, const PGAndroidPipelineLayout &layout,
    const uint32_t *vertexBufferIds, const VkDeviceSize *vertexOffsets,
    uint32_t vertexBufferCount, uint32_t indexBufferId, VkDeviceSize indexOffset,
    VkIndexType indexType, uint32_t vertexCount, uint32_t indexCount,
    uint32_t instanceCount, uint32_t firstVertexOrIndex, int32_t vertexOffset,
    uint32_t firstInstance);
