#include "gpu_render_targets.h"

#include <vector>

bool PGAndroidCreateRenderTargets(VkDevice device, const PGAndroidGpuResources &resources,
    const uint32_t *colorViewIds, uint32_t colorCount, uint32_t depthViewId,
    uint32_t width, uint32_t height, PGAndroidRenderTargets &out) {
    if (device==VK_NULL_HANDLE || (!colorViewIds && colorCount) || colorCount>8 ||
        (colorCount==0 && depthViewId==0) || !width || !height ||
        out.renderPass || out.framebuffer) return false;

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    std::vector<VkImageView> imageViews;
    attachments.reserve(colorCount+(depthViewId?1u:0u));
    colorRefs.reserve(colorCount);
    imageViews.reserve(colorCount+(depthViewId?1u:0u));
    VkSampleCountFlagBits commonSamples=VK_SAMPLE_COUNT_1_BIT;
    bool haveSamples=false;

    PGAndroidRenderTargets result{};
    result.width=width; result.height=height;
    result.colorCount=colorCount; result.hasDepth=depthViewId!=0;
    result.depthViewId=depthViewId;
    for (uint32_t i=0;i<colorCount;++i) {
        if (!colorViewIds[i]) return false;
        for (uint32_t j=0;j<i;++j) if (colorViewIds[j]==colorViewIds[i]) return false;
        VkImageView view=VK_NULL_HANDLE; VkFormat format=VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT;
        if (!resources.getAttachmentInfo(colorViewIds[i],false,view,format,samples)) return false;
        if (haveSamples && samples!=commonSamples) return false;
        commonSamples=samples; haveSamples=true;
        result.colorViewIds[i]=colorViewIds[i];
        VkAttachmentDescription attachment{};
        attachment.format=format; attachment.samples=samples;
        attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
        colorRefs.push_back({i,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        imageViews.push_back(view);
    }

    VkAttachmentReference depthRef{};
    if (depthViewId) {
        VkImageView view=VK_NULL_HANDLE; VkFormat format=VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT;
        if (!resources.getAttachmentInfo(depthViewId,true,view,format,samples) ||
            (haveSamples && samples!=commonSamples)) return false;
        VkAttachmentDescription attachment{};
        attachment.format=format; attachment.samples=samples;
        attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.initialLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthRef={static_cast<uint32_t>(attachments.size()),VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        attachments.push_back(attachment);
        imageViews.push_back(view);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount=static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments=colorRefs.empty()?nullptr:colorRefs.data();
    subpass.pDepthStencilAttachment=depthViewId?&depthRef:nullptr;
    VkSubpassDependency dependency{};
    dependency.srcSubpass=VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass=0;
    dependency.srcStageMask=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    dependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        (depthViewId ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : 0);
    dependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        (depthViewId ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0);
    VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    passInfo.attachmentCount=static_cast<uint32_t>(attachments.size());
    passInfo.pAttachments=attachments.data();
    passInfo.subpassCount=1; passInfo.pSubpasses=&subpass;
    passInfo.dependencyCount=1; passInfo.pDependencies=&dependency;
    if (vkCreateRenderPass(device,&passInfo,nullptr,&result.renderPass)!=VK_SUCCESS) return false;
    VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbInfo.renderPass=result.renderPass;
    fbInfo.attachmentCount=static_cast<uint32_t>(imageViews.size());
    fbInfo.pAttachments=imageViews.data();
    fbInfo.width=width; fbInfo.height=height; fbInfo.layers=1;
    if (vkCreateFramebuffer(device,&fbInfo,nullptr,&result.framebuffer)!=VK_SUCCESS) {
        vkDestroyRenderPass(device,result.renderPass,nullptr);
        return false;
    }
    out=result;
    return true;
}

bool PGAndroidBeginRenderTargets(VkCommandBuffer commandBuffer,
    PGAndroidGpuResources &resources, const uint32_t *colorViewIds,
    uint32_t colorCount, uint32_t depthViewId, const PGAndroidRenderTargets &targets,
    const VkClearColorValue *colorClears, float depthClear, uint32_t stencilClear) {
    if (commandBuffer==VK_NULL_HANDLE || !targets.renderPass || !targets.framebuffer ||
        colorCount!=targets.colorCount || (colorCount && !colorViewIds) ||
        (depthViewId!=0)!=targets.hasDepth || depthViewId!=targets.depthViewId ||
        depthClear<0.0f || depthClear>1.0f) return false;
    for (uint32_t i=0;i<colorCount;++i)
        if (!colorViewIds[i] || colorViewIds[i]!=targets.colorViewIds[i]) return false;

    std::vector<VkClearValue> clears(colorCount+(depthViewId?1u:0u));
    for (uint32_t i=0;i<colorCount;++i) {
        VkImageView view=VK_NULL_HANDLE; VkFormat format=VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT;
        if (!resources.prepareAttachment(commandBuffer,colorViewIds[i],false,view,format,samples))
            return false;
        if (colorClears) clears[i].color=colorClears[i];
        else {
            clears[i].color.float32[0]=0.0f;
            clears[i].color.float32[1]=0.0f;
            clears[i].color.float32[2]=0.0f;
            clears[i].color.float32[3]=1.0f;
        }
    }
    if (depthViewId) {
        VkImageView view=VK_NULL_HANDLE; VkFormat format=VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT;
        if (!resources.prepareAttachment(commandBuffer,depthViewId,true,view,format,samples))
            return false;
        clears[colorCount].depthStencil={depthClear,stencilClear};
    }
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin.renderPass=targets.renderPass; begin.framebuffer=targets.framebuffer;
    begin.renderArea={{0,0},{targets.width,targets.height}};
    begin.clearValueCount=static_cast<uint32_t>(clears.size());
    begin.pClearValues=clears.data();
    vkCmdBeginRenderPass(commandBuffer,&begin,VK_SUBPASS_CONTENTS_INLINE);
    return true;
}

void PGAndroidDestroyRenderTargets(VkDevice device, PGAndroidRenderTargets &targets) {
    if (device==VK_NULL_HANDLE) return;
    if (targets.framebuffer) vkDestroyFramebuffer(device,targets.framebuffer,nullptr);
    if (targets.renderPass) vkDestroyRenderPass(device,targets.renderPass,nullptr);
    targets={};
}
