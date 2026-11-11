#include "gpu_draw_commands.h"

#include <vector>

bool PGAndroidRecordDraw(VkCommandBuffer commandBuffer,
    const PGAndroidPipeline &pipeline, const PGAndroidPipelineLayout &layout,
    const PGAndroidGpuResources &resources,
    const uint32_t *vertexBufferIds, const VkDeviceSize *vertexOffsets,
    uint32_t vertexBufferCount, uint32_t indexBufferId, VkDeviceSize indexOffset,
    VkIndexType indexType, uint32_t vertexCount, uint32_t indexCount,
    uint32_t instanceCount, uint32_t firstVertexOrIndex, int32_t vertexOffset,
    uint32_t firstInstance) {
    if (!commandBuffer || !pipeline.handle || pipeline.compute ||
        !layout.pipelineLayout || !layout.descriptorSet ||
        (!vertexBufferIds && vertexBufferCount) || (!vertexOffsets && vertexBufferCount) ||
        vertexBufferCount > 16 || instanceCount == 0 ||
        (!indexBufferId && vertexCount == 0)) return false;

    std::vector<VkBuffer> buffers(vertexBufferCount);
    for (uint32_t i=0;i<vertexBufferCount;++i) {
        VkDeviceSize size=0;
        if (!resources.getBufferHandle(vertexBufferIds[i],0x01u,buffers[i],size) ||
            vertexOffsets[i] >= size) return false;
    }

    VkBuffer indexBuffer=VK_NULL_HANDLE;
    if (indexBufferId) {
        VkDeviceSize size=0;
        const VkDeviceSize elementSize=indexType==VK_INDEX_TYPE_UINT16 ? 2u :
                                       indexType==VK_INDEX_TYPE_UINT32 ? 4u : 0u;
        if (!elementSize || (indexOffset % elementSize) ||
            !resources.getBufferHandle(indexBufferId,0x02u,indexBuffer,size) ||
            indexOffset > size || static_cast<VkDeviceSize>(indexCount) > (size-indexOffset)/elementSize) return false;
    } else if (indexCount != 0) {
        return false;
    }

    vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline.handle);
    vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout.pipelineLayout,0,1,&layout.descriptorSet,0,nullptr);
    if (!buffers.empty())
        vkCmdBindVertexBuffers(commandBuffer,0,vertexBufferCount,buffers.data(),vertexOffsets);
    if (indexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer,indexBuffer,indexOffset,indexType);
        vkCmdDrawIndexed(commandBuffer,indexCount,instanceCount,firstVertexOrIndex,
                         vertexOffset,firstInstance);
    } else {
        vkCmdDraw(commandBuffer,vertexCount,instanceCount,firstVertexOrIndex,firstInstance);
    }
    return true;
}

bool PGAndroidRecordDispatch(VkCommandBuffer commandBuffer,
    const PGAndroidPipeline &pipeline, const PGAndroidPipelineLayout &layout,
    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    if (!commandBuffer || !pipeline.handle || !pipeline.compute ||
        !layout.pipelineLayout || !layout.descriptorSet ||
        !groupCountX || !groupCountY || !groupCountZ) return false;
    vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle);
    vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,
        layout.pipelineLayout,0,1,&layout.descriptorSet,0,nullptr);
    vkCmdDispatch(commandBuffer,groupCountX,groupCountY,groupCountZ);
    return true;
}
