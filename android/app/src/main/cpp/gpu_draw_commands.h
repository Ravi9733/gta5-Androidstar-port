#pragma once

#include "gpu_pipeline.h"
#include "gpu_resources.h"

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
