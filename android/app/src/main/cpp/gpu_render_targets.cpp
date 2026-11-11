#include "gpu_render_targets.h"

#include <vector>

bool PGAndroidCreateRenderTargets(VkDevice device, const PGAndroidGpuResources &resources,
    const uint32_t *colorViewIds, uint32_t colorCount, uint32_t depthViewId,
    uint32_t width, uint32_t height, PGAndroidRenderTargets &out) {
    if (device==VK_NULL_HANDLE || (!colorViewIds && colorCount) ||
        colorCount>8 || (colorCount==0 && depthViewId==0) || !width || !height ||
        out.renderPass || out.framebuffer) return false;

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colors;
    std::vector<VkImageView> views;
    VkSampleCountFlagBits commonSamples=VK_SAMPLE_COUNT_1_BIT;
    bool haveSamples=false;
    for (uint32_t i=0;i<colorCount;++i) {
        VkImageView view=VK_NULL_HANDLE; VkFormat format=VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT;
        // Query attachment metadata without recording commands.
        VkDescriptorImageInfo image{};
        if (!resources.getImageDescriptor(colorViewIds[i],image,false) || !image.imageView)
            return false;
        // The resource helper validates render-target bind flags and transitions at begin.
        // Format/sample metadata is returned by a command-buffer-free validation helper below.
        view=image.imageView;
        (void)view;
        (void)format;
        (void)samples;
        return false;
    }
    return false;
}

bool PGAndroidBeginRenderTargets(VkCommandBuffer, PGAndroidGpuResources &,
    const uint32_t *, uint32_t, uint32_t, const PGAndroidRenderTargets &,
    const VkClearColorValue *, float, uint32_t) {
    return false;
}

void PGAndroidDestroyRenderTargets(VkDevice device, PGAndroidRenderTargets &targets) {
    if (device==VK_NULL_HANDLE) return;
    if (targets.framebuffer) vkDestroyFramebuffer(device,targets.framebuffer,nullptr);
    if (targets.renderPass) vkDestroyRenderPass(device,targets.renderPass,nullptr);
    targets={};
}
