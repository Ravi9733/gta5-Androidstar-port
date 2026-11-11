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
        !layout.pipelineLayout ||
        (!vertexBufferIds && vertexBufferCount) || (!vertexOffsets && vertexBufferCount) ||
        vertexBufferCount == 0 || vertexBufferCount > 16 || instanceCount == 0) return false;

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
        const uint64_t endIndex=static_cast<uint64_t>(firstVertexOrIndex)+indexCount;
        if (!elementSize || indexCount == 0 || endIndex > UINT32_MAX ||
            (indexOffset % elementSize) ||
            !resources.getBufferHandle(indexBufferId,0x02u,indexBuffer,size) ||
            indexOffset > size ||
            endIndex > (size-indexOffset)/elementSize) return false;
    } else {
        if (indexCount != 0 || vertexCount == 0 ||
            static_cast<uint64_t>(firstVertexOrIndex)+vertexCount > UINT32_MAX) return false;
    }

    vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline.handle);
    vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout.pipelineLayout,0,3,layout.descriptorSets.data(),0,nullptr);
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
        !layout.pipelineLayout ||
        !groupCountX || !groupCountY || !groupCountZ) return false;
    vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline.handle);
    vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,
        layout.pipelineLayout,0,3,layout.descriptorSets.data(),0,nullptr);
    vkCmdDispatch(commandBuffer,groupCountX,groupCountY,groupCountZ);
    return true;
}

bool PGAndroidRecordDrawInRenderTargets(VkCommandBuffer commandBuffer,
    PGAndroidGpuResources &resources, const PGAndroidRenderTargets &targets,
    const uint32_t *colorViewIds, uint32_t colorCount, uint32_t depthViewId,
    const VkClearColorValue *colorClears, float depthClear, uint32_t stencilClear,
    const PGAndroidPipeline &pipeline, const PGAndroidPipelineLayout &layout,
    const uint32_t *vertexBufferIds, const VkDeviceSize *vertexOffsets,
    uint32_t vertexBufferCount, uint32_t indexBufferId, VkDeviceSize indexOffset,
    VkIndexType indexType, uint32_t vertexCount, uint32_t indexCount,
    uint32_t instanceCount, uint32_t firstVertexOrIndex, int32_t vertexOffset,
    uint32_t firstInstance) {
    if (!PGAndroidBeginRenderTargets(commandBuffer,resources,colorViewIds,colorCount,
        depthViewId,targets,colorClears,depthClear,stencilClear)) return false;
    const bool recorded=PGAndroidRecordDraw(commandBuffer,pipeline,layout,resources,
        vertexBufferIds,vertexOffsets,vertexBufferCount,indexBufferId,indexOffset,indexType,
        vertexCount,indexCount,instanceCount,firstVertexOrIndex,vertexOffset,firstInstance);
    vkCmdEndRenderPass(commandBuffer);
    return recorded;
}

bool PGAndroidPrepareAndRecordDraw(VkCommandBuffer commandBuffer,VkDevice device,
    PGAndroidGpuResources &resources,const PGAndroidRenderTargets &targets,
    const uint32_t *colorViewIds,uint32_t colorCount,uint32_t depthViewId,
    const VkClearColorValue *colorClears,float depthClear,uint32_t stencilClear,
    const PGAndroidPipeline &pipeline,const PGAndroidPipelineLayout &layout,
    const PGAndroidShaderModule *shaders,uint32_t shaderCount,
    const PGAndroidGpuBindingState &state,PGAndroidDescriptorWriter &writer,
    const uint32_t *vertexBufferIds,const VkDeviceSize *vertexOffsets,
    uint32_t vertexBufferCount,uint32_t indexBufferId,VkDeviceSize indexOffset,
    VkIndexType indexType,uint32_t vertexCount,uint32_t indexCount,
    uint32_t instanceCount,uint32_t firstVertexOrIndex,int32_t vertexOffset,
    uint32_t firstInstance) {
    if(!PGAndroidPreparePipelineDescriptors(commandBuffer,device,layout,shaders,
        shaderCount,state,resources,writer)) return false;
    return PGAndroidRecordDrawInRenderTargets(commandBuffer,resources,targets,
        colorViewIds,colorCount,depthViewId,colorClears,depthClear,stencilClear,
        pipeline,layout,vertexBufferIds,vertexOffsets,vertexBufferCount,indexBufferId,
        indexOffset,indexType,vertexCount,indexCount,instanceCount,firstVertexOrIndex,
        vertexOffset,firstInstance);
}

bool PGAndroidPrepareAndRecordDispatch(VkCommandBuffer commandBuffer,VkDevice device,
    PGAndroidGpuResources &resources,const PGAndroidPipeline &pipeline,
    const PGAndroidPipelineLayout &layout,const PGAndroidShaderModule &shader,
    const PGAndroidGpuBindingState &state,PGAndroidDescriptorWriter &writer,
    uint32_t groupCountX,uint32_t groupCountY,uint32_t groupCountZ) {
    if(!PGAndroidPreparePipelineDescriptors(commandBuffer,device,layout,&shader,1,
        state,resources,writer)) return false;
    return PGAndroidRecordDispatch(commandBuffer,pipeline,layout,
        groupCountX,groupCountY,groupCountZ);
}
