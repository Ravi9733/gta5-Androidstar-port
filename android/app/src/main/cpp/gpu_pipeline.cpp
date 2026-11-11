#include "gpu_pipeline.h"

#include <vector>

bool PGAndroidCreateGraphicsPipeline(VkDevice device, VkPipelineCache cache,
    const PGAndroidPipelineLayout &layout, VkRenderPass renderPass,
    const PGAndroidShaderModule *shaders, uint32_t shaderCount,
    const VkGraphicsPipelineCreateInfo &createInfo, PGAndroidPipeline &out) {
    if (!device || !layout.pipelineLayout || !renderPass || !shaders ||
        shaderCount != 2 || !createInfo.pStages || createInfo.stageCount != shaderCount ||
        !createInfo.pVertexInputState || !createInfo.pInputAssemblyState ||
        !createInfo.pViewportState || !createInfo.pRasterizationState ||
        !createInfo.pMultisampleState || !createInfo.pColorBlendState ||
        out.handle) return false;

    bool hasVertex=false, hasFragment=false;
    for (uint32_t i=0;i<shaderCount;++i) {
        if (!shaders[i].module || (shaders[i].stage != VK_SHADER_STAGE_VERTEX_BIT &&
            shaders[i].stage != VK_SHADER_STAGE_FRAGMENT_BIT)) return false;
        hasVertex |= shaders[i].stage == VK_SHADER_STAGE_VERTEX_BIT;
        hasFragment |= shaders[i].stage == VK_SHADER_STAGE_FRAGMENT_BIT;
        bool matched=false;
        for (uint32_t j=0;j<createInfo.stageCount;++j)
            if (createInfo.pStages[j].module == shaders[i].module &&
                createInfo.pStages[j].stage == shaders[i].stage) matched=true;
        if (!matched) return false;
    }
    if (!hasVertex || !hasFragment) return false;

    VkGraphicsPipelineCreateInfo info=createInfo;
    info.layout=layout.pipelineLayout;
    info.renderPass=renderPass;
    info.subpass=0;
    VkPipeline pipeline=VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(device,cache,1,&info,nullptr,&pipeline)!=VK_SUCCESS)
        return false;
    out.handle=pipeline;
    out.compute=false;
    return true;
}

bool PGAndroidCreateComputePipeline(VkDevice device, VkPipelineCache cache,
    const PGAndroidPipelineLayout &layout, const PGAndroidShaderModule &shader,
    const VkComputePipelineCreateInfo &createInfo, PGAndroidPipeline &out) {
    if (!device || !layout.pipelineLayout || !shader.module ||
        shader.stage != VK_SHADER_STAGE_COMPUTE_BIT ||
        createInfo.stage.module != shader.module ||
        createInfo.stage.stage != VK_SHADER_STAGE_COMPUTE_BIT || out.handle) return false;
    VkComputePipelineCreateInfo info=createInfo;
    info.layout=layout.pipelineLayout;
    VkPipeline pipeline=VK_NULL_HANDLE;
    if (vkCreateComputePipelines(device,cache,1,&info,nullptr,&pipeline)!=VK_SUCCESS)
        return false;
    out.handle=pipeline;
    out.compute=true;
    return true;
}

void PGAndroidDestroyPipeline(VkDevice device, PGAndroidPipeline &pipeline) {
    if (device && pipeline.handle) vkDestroyPipeline(device,pipeline.handle,nullptr);
    pipeline.handle=VK_NULL_HANDLE;
    pipeline.compute=false;
}
