#define VK_USE_PLATFORM_ANDROID_KHR
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include "gpu_resources.h"
#include "gpu_descriptor_pool.h"
#include "gpu_binding_state.h"
#include "PGAndroidGpuProtocol.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <memory>
#include <vector>

extern "C" void PGStopGraphics(void);

namespace {
constexpr const char *kTag = "GTA5Android";
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, kTag, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)

struct VulkanHost {
    std::mutex mutex;
    ANativeWindow *window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    std::unique_ptr<PGAndroidGpuResources> resources;
    std::unique_ptr<PGAndroidDescriptorPool> descriptorPool;
    PGAndroidGpuBindingState bindings;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamily = UINT32_MAX;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffers[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkSemaphore imageAvailable[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkSemaphore renderFinished[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkFence inFlight[2]{VK_NULL_HANDLE, VK_NULL_HANDLE};
    uint32_t frameIndex = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
} host;

bool check(VkResult result, const char *operation) {
    if (result == VK_SUCCESS) return true;
    LOGE("%s failed: VkResult %d", operation, static_cast<int>(result));
    return false;
}

bool supportsSwapchain(VkPhysicalDevice device) {
    uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS) return false;
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS) return false;
    for (const auto &extension : extensions)
        if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) return true;
    return false;
}

void destroySwapchain() {
    if (host.device) {
        (void)vkDeviceWaitIdle(host.device);
        for (VkFramebuffer framebuffer : host.framebuffers)
            if (framebuffer) vkDestroyFramebuffer(host.device, framebuffer, nullptr);
        host.framebuffers.clear();
        if (host.renderPass) vkDestroyRenderPass(host.device, host.renderPass, nullptr);
        host.renderPass = VK_NULL_HANDLE;
        for (VkImageView view : host.swapchainViews)
            if (view) vkDestroyImageView(host.device, view, nullptr);
        host.swapchainViews.clear();
        host.swapchainImages.clear();
        if (host.swapchain) vkDestroySwapchainKHR(host.device, host.swapchain, nullptr);
    }
    host.swapchain = VK_NULL_HANDLE;
}

void destroyLocked() {
    destroySwapchain();
    if (host.device) {
        (void)vkDeviceWaitIdle(host.device);
        host.descriptorPool.reset();
        host.resources.reset();
        for (size_t i = 0; i < 2; ++i) {
            if (host.inFlight[i]) vkDestroyFence(host.device, host.inFlight[i], nullptr);
            if (host.imageAvailable[i]) vkDestroySemaphore(host.device, host.imageAvailable[i], nullptr);
            if (host.renderFinished[i]) vkDestroySemaphore(host.device, host.renderFinished[i], nullptr);
        }
        if (host.commandPool) vkDestroyCommandPool(host.device, host.commandPool, nullptr);
        host.commandPool = VK_NULL_HANDLE;
        host.inFlight[0] = host.inFlight[1] = VK_NULL_HANDLE;
        host.imageAvailable[0] = host.imageAvailable[1] = VK_NULL_HANDLE;
        host.renderFinished[0] = host.renderFinished[1] = VK_NULL_HANDLE;
        host.commandBuffers[0] = host.commandBuffers[1] = VK_NULL_HANDLE;
        vkDestroyDevice(host.device, nullptr);
    }
    host.device = VK_NULL_HANDLE;
    host.queue = VK_NULL_HANDLE;
    host.physical = VK_NULL_HANDLE;
    host.queueFamily = UINT32_MAX;
    if (host.instance && host.surface) vkDestroySurfaceKHR(host.instance, host.surface, nullptr);
    host.surface = VK_NULL_HANDLE;
    if (host.instance) vkDestroyInstance(host.instance, nullptr);
    host.instance = VK_NULL_HANDLE;
    if (host.window) ANativeWindow_release(host.window);
    host.window = nullptr;
}

bool makeSwapchain(uint32_t requestedWidth, uint32_t requestedHeight) {
    if (!host.device || !host.surface) return false;
    VkSurfaceCapabilitiesKHR caps{};
    if (!check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(host.physical, host.surface, &caps),
               "vkGetPhysicalDeviceSurfaceCapabilitiesKHR")) return false;
    if (!(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        LOGE("Surface does not support color-attachment swapchain images");
        return false;
    }

    uint32_t formatCount = 0;
    if (!check(vkGetPhysicalDeviceSurfaceFormatsKHR(host.physical, host.surface, &formatCount, nullptr),
               "vkGetPhysicalDeviceSurfaceFormatsKHR") || formatCount == 0) return false;
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    if (!check(vkGetPhysicalDeviceSurfaceFormatsKHR(host.physical, host.surface, &formatCount, formats.data()),
               "vkGetPhysicalDeviceSurfaceFormatsKHR")) return false;

    VkSurfaceFormatKHR selected = formats.front();
    if (formats.size() == 1 && formats.front().format == VK_FORMAT_UNDEFINED)
        selected.format = VK_FORMAT_B8G8R8A8_UNORM;
    for (const auto &candidate : formats) {
        if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM &&
            candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            selected = candidate;
            break;
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent.width = std::clamp(requestedWidth, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(requestedHeight, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    if (extent.width == 0 || extent.height == 0) return false;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = host.surface;
    info.minImageCount = imageCount;
    info.imageFormat = selected.format;
    info.imageColorSpace = selected.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    const VkCompositeAlphaFlagBitsKHR alphaModes[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for (VkCompositeAlphaFlagBitsKHR alpha : alphaModes) {
        if (caps.supportedCompositeAlpha & alpha) { info.compositeAlpha = alpha; break; }
    }
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;
    info.oldSwapchain = host.swapchain;

    VkSwapchainKHR replacement = VK_NULL_HANDLE;
    if (!check(vkCreateSwapchainKHR(host.device, &info, nullptr, &replacement), "vkCreateSwapchainKHR"))
        return false;
    destroySwapchain();
    host.swapchain = replacement;
    host.format = selected.format;
    host.extent = extent;

    uint32_t actualImageCount = 0;
    if (!check(vkGetSwapchainImagesKHR(host.device, host.swapchain, &actualImageCount, nullptr),
               "vkGetSwapchainImagesKHR(count)") || actualImageCount == 0) {
        destroySwapchain();
        return false;
    }
    host.swapchainImages.resize(actualImageCount);
    if (!check(vkGetSwapchainImagesKHR(host.device, host.swapchain, &actualImageCount,
                                       host.swapchainImages.data()),
               "vkGetSwapchainImagesKHR(data)")) {
        destroySwapchain();
        return false;
    }
    host.swapchainImages.resize(actualImageCount);
    host.swapchainViews.reserve(actualImageCount);
    for (VkImage image : host.swapchainImages) {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = host.format;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VkImageView view = VK_NULL_HANDLE;
        if (!check(vkCreateImageView(host.device, &viewInfo, nullptr, &view),
                   "vkCreateImageView(swapchain)")) {
            destroySwapchain();
            return false;
        }
        host.swapchainViews.push_back(view);
    }
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = host.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference colorReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if (!check(vkCreateRenderPass(host.device, &renderPassInfo, nullptr, &host.renderPass),
               "vkCreateRenderPass")) {
        destroySwapchain();
        return false;
    }
    for (VkImageView view : host.swapchainViews) {
        VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = host.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &view;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (!check(vkCreateFramebuffer(host.device, &framebufferInfo, nullptr, &framebuffer),
                   "vkCreateFramebuffer")) {
            destroySwapchain();
            return false;
        }
        host.framebuffers.push_back(framebuffer);
    }
    LOGI("Vulkan swapchain ready: %ux%u, %u images/views, format %d",
         extent.width, extent.height, actualImageCount, static_cast<int>(selected.format));
    return true;
}

bool initialize(ANativeWindow *window) {
    host.window = window;
    const char *instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "GTA V Android Port";
    app.applicationVersion = 1;
    app.pEngineName = "Original iOS port Android host";
    app.engineVersion = 1;
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &app;
    instanceInfo.enabledExtensionCount = 2;
    instanceInfo.ppEnabledExtensionNames = instanceExtensions;
    if (!check(vkCreateInstance(&instanceInfo, nullptr, &host.instance), "vkCreateInstance")) return false;

    VkAndroidSurfaceCreateInfoKHR surfaceInfo{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    surfaceInfo.window = host.window;
    if (!check(vkCreateAndroidSurfaceKHR(host.instance, &surfaceInfo, nullptr, &host.surface),
               "vkCreateAndroidSurfaceKHR")) return false;

    uint32_t physicalCount = 0;
    if (!check(vkEnumeratePhysicalDevices(host.instance, &physicalCount, nullptr), "vkEnumeratePhysicalDevices") ||
        physicalCount == 0) return false;
    std::vector<VkPhysicalDevice> devices(physicalCount);
    if (!check(vkEnumeratePhysicalDevices(host.instance, &physicalCount, devices.data()),
               "vkEnumeratePhysicalDevices")) return false;

    for (VkPhysicalDevice candidate : devices) {
        if (!supportsSwapchain(candidate)) continue;
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
        for (uint32_t i = 0; i < familyCount; ++i) {
            VkBool32 present = VK_FALSE;
            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, host.surface, &present) == VK_SUCCESS &&
                present) {
                host.physical = candidate;
                host.queueFamily = i;
                break;
            }
        }
        if (host.physical) break;
    }
    if (!host.physical) { LOGE("No Vulkan graphics+present queue found"); return false; }

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = host.queueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(host.physical, &supportedFeatures);
    VkPhysicalDeviceFeatures enabledFeatures{};
    enabledFeatures.textureCompressionBC = supportedFeatures.textureCompressionBC;
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.pEnabledFeatures = &enabledFeatures;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    if (!check(vkCreateDevice(host.physical, &deviceInfo, nullptr, &host.device), "vkCreateDevice")) return false;
    vkGetDeviceQueue(host.device, host.queueFamily, 0, &host.queue);
    host.resources = std::make_unique<PGAndroidGpuResources>(host.physical, host.device, host.queue, host.queueFamily,
            supportedFeatures.textureCompressionBC == VK_TRUE);
    host.descriptorPool = std::make_unique<PGAndroidDescriptorPool>();
    if (!host.descriptorPool->initialize(host.device, 64, 64)) {
        LOGE("Failed to initialize Vulkan descriptor pool");
        return false;
    }

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = host.queueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (!check(vkCreateCommandPool(host.device, &poolInfo, nullptr, &host.commandPool),
               "vkCreateCommandPool")) return false;
    VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = host.commandPool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = 2;
    if (!check(vkAllocateCommandBuffers(host.device, &commandInfo, host.commandBuffers),
               "vkAllocateCommandBuffers")) return false;
    for (size_t i = 0; i < 2; ++i) {
        VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (!check(vkCreateSemaphore(host.device, &semaphoreInfo, nullptr, &host.imageAvailable[i]),
                   "vkCreateSemaphore(image)") ||
            !check(vkCreateSemaphore(host.device, &semaphoreInfo, nullptr, &host.renderFinished[i]),
                   "vkCreateSemaphore(render)") ||
            !check(vkCreateFence(host.device, &fenceInfo, nullptr, &host.inFlight[i]),
                   "vkCreateFence")) return false;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(host.physical, &properties);
    LOGI("Selected Vulkan GPU: %s (API %u.%u.%u)", properties.deviceName,
         VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion),
         VK_VERSION_PATCH(properties.apiVersion));
    return makeSwapchain(static_cast<uint32_t>(ANativeWindow_getWidth(host.window)),
                         static_cast<uint32_t>(ANativeWindow_getHeight(host.window)));
}
} // namespace

extern "C" JNIEXPORT jboolean JNICALL
Java_com_ravi_gta5port_MainActivity_nativeCreate(JNIEnv *env, jclass, jobject surface) {
    std::lock_guard<std::mutex> lock(host.mutex);
    destroyLocked();
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    if (!window) { LOGE("ANativeWindow_fromSurface returned null"); return JNI_FALSE; }
    if (initialize(window)) return JNI_TRUE;
    destroyLocked();
    return JNI_FALSE;
}


extern "C" JNIEXPORT jboolean JNICALL
Java_com_ravi_gta5port_MainActivity_nativeRenderFrame(JNIEnv *, jclass) {
    std::lock_guard<std::mutex> lock(host.mutex);
    if (!host.device || !host.swapchain || !host.renderPass || !host.commandPool)
        return JNI_FALSE;
    const uint32_t slot = host.frameIndex;
    if (!check(vkWaitForFences(host.device, 1, &host.inFlight[slot], VK_TRUE, UINT64_MAX),
               "vkWaitForFences")) return JNI_FALSE;
    uint32_t imageIndex = 0;
    VkResult acquired = vkAcquireNextImageKHR(host.device, host.swapchain, UINT64_MAX,
        host.imageAvailable[slot], VK_NULL_HANDLE, &imageIndex);
    if (acquired == VK_ERROR_OUT_OF_DATE_KHR) {
        (void)makeSwapchain(static_cast<uint32_t>(ANativeWindow_getWidth(host.window)),
                            static_cast<uint32_t>(ANativeWindow_getHeight(host.window)));
        return JNI_FALSE;
    }
    if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR) {
        check(acquired, "vkAcquireNextImageKHR");
        return JNI_FALSE;
    }
    if (imageIndex >= host.framebuffers.size()) return JNI_FALSE;
    if (!check(vkResetCommandBuffer(host.commandBuffers[slot], 0), "vkResetCommandBuffer"))
        return JNI_FALSE;
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (!check(vkBeginCommandBuffer(host.commandBuffers[slot], &begin), "vkBeginCommandBuffer"))
        return JNI_FALSE;
    VkClearValue clear{};
    clear.color = {{0.025f, 0.045f, 0.075f, 1.0f}};
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = host.renderPass;
    pass.framebuffer = host.framebuffers[imageIndex];
    pass.renderArea.extent = host.extent;
    pass.clearValueCount = 1;
    pass.pClearValues = &clear;
    vkCmdBeginRenderPass(host.commandBuffers[slot], &pass, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(host.commandBuffers[slot]);
    if (!check(vkEndCommandBuffer(host.commandBuffers[slot]), "vkEndCommandBuffer"))
        return JNI_FALSE;
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &host.imageAvailable[slot];
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &host.commandBuffers[slot];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &host.renderFinished[slot];
    // Reset only when a complete command buffer is ready to submit. If recording
    // fails earlier, the fence remains signaled and the next frame cannot deadlock.
    if (!check(vkResetFences(host.device, 1, &host.inFlight[slot]), "vkResetFences"))
        return JNI_FALSE;
    VkResult submitted = vkQueueSubmit(host.queue, 1, &submit, host.inFlight[slot]);
    if (submitted != VK_SUCCESS) {
        check(submitted, "vkQueueSubmit");
        // A failed submit will never signal the reset fence; replace it in the
        // signaled state so a later frame can recover instead of waiting forever.
        vkDestroyFence(host.device, host.inFlight[slot], nullptr);
        VkFenceCreateInfo recovery{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        recovery.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        host.inFlight[slot] = VK_NULL_HANDLE;
        if (!check(vkCreateFence(host.device, &recovery, nullptr, &host.inFlight[slot]),
                   "vkCreateFence(recovery)")) return JNI_FALSE;
        return JNI_FALSE;
    }
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &host.renderFinished[slot];
    present.swapchainCount = 1;
    present.pSwapchains = &host.swapchain;
    present.pImageIndices = &imageIndex;
    VkResult shown = vkQueuePresentKHR(host.queue, &present);
    host.frameIndex = (slot + 1) % 2;
    if (shown == VK_ERROR_OUT_OF_DATE_KHR || shown == VK_SUBOPTIMAL_KHR ||
        acquired == VK_SUBOPTIMAL_KHR) {
        (void)makeSwapchain(static_cast<uint32_t>(ANativeWindow_getWidth(host.window)),
                            static_cast<uint32_t>(ANativeWindow_getHeight(host.window)));
        return JNI_FALSE;
    }
    if (!check(shown, "vkQueuePresentKHR")) return JNI_FALSE;
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_ravi_gta5port_MainActivity_nativeResize(JNIEnv *, jclass, jint width, jint height) {
    std::lock_guard<std::mutex> lock(host.mutex);
    if (width <= 0 || height <= 0) return JNI_FALSE;
    return makeSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height)) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_ravi_gta5port_MainActivity_nativeDestroy(JNIEnv *, jclass) {
    PGStopGraphics();
    std::lock_guard<std::mutex> lock(host.mutex);
    destroyLocked();
}

// Keep this ABI layout in sync with PGControllerState in app/Source/EngineHost.h.
// EngineHost.c is compiled as C, so the optional symbol must use C language linkage.
extern "C" {
struct PGControllerState {
    float leftX, leftY, rightX, rightY, leftTrigger, rightTrigger;
    uint32_t buttons;
    int32_t lookDX, lookDY;
};
void PGUpdateController(PGControllerState state, double elapsedSeconds) __attribute__((weak));
}

extern "C" JNIEXPORT void JNICALL
Java_com_ravi_gta5port_MainActivity_nativeUpdateController(
    JNIEnv *, jclass, jfloat leftX, jfloat leftY, jfloat rightX, jfloat rightY,
    jfloat leftTrigger, jfloat rightTrigger, jint buttons, jint lookDX, jint lookDY,
    jdouble elapsedSeconds) {
    if (!PGUpdateController) return; // EngineHost is linked in a later integration stage.
    PGControllerState state{};
    state.leftX = leftX;
    state.leftY = leftY;
    state.rightX = rightX;
    state.rightY = rightY;
    state.leftTrigger = leftTrigger;
    state.rightTrigger = rightTrigger;
    state.buttons = static_cast<uint32_t>(buttons);
    state.lookDX = lookDX;
    state.lookDY = lookDY;
    PGUpdateController(state, elapsedSeconds);
}


/* Entry point for the original iOS command-ring consumer once its shared
 * engine-memory ring is attached to the Android host. Packet words exclude
 * the 8-byte command header, matching MRRenderer.mm execute(op, p, n). */
extern "C" bool PGAndroidExecuteGpuPacket(uint32_t opcode, const uint32_t *words,
                                           uint32_t wordCount) {
    std::lock_guard<std::mutex> lock(host.mutex);
    if (!host.device || !host.resources || (!words && wordCount)) return false;
    switch (opcode) {
        case PG_ANDROID_OP_INIT:
            host.bindings.clear();
            return true;
        case PG_ANDROID_OP_CREATE_STATE:
            return host.resources->createSamplerState(words, wordCount);
        case PG_ANDROID_OP_CREATE_TEXTURE:
            return host.resources->createTexture(words, wordCount);
        case PG_ANDROID_OP_CREATE_SRV:
            return host.resources->createTextureView(words, wordCount, false);
        case PG_ANDROID_OP_CREATE_UAV:
            return host.resources->createTextureView(words, wordCount, true);
        case PG_ANDROID_OP_CREATE_BUFFER:
            return host.resources->createBuffer(words, wordCount);
        case PG_ANDROID_OP_UPLOAD_TEXTURE:
            return host.resources->uploadTexture(words, wordCount);
        case PG_ANDROID_OP_UPLOAD_BUFFER:
        case PG_ANDROID_OP_UPLOAD_BUFFER_NO_OVERWRITE:
            return host.resources->uploadBuffer(words, wordCount);
        case PG_ANDROID_OP_DESTROY_OBJECT:
            return wordCount >= 1 && host.resources->destroyObject(words[0]);
        case PG_ANDROID_OP_SET_CBUFFERS:
            return host.bindings.setConstantBuffers(words, wordCount);
        case PG_ANDROID_OP_SET_SRVS:
            return host.bindings.setShaderResources(words, wordCount);
        case PG_ANDROID_OP_SET_SAMPLERS:
            return host.bindings.setSamplers(words, wordCount);
        case PG_ANDROID_OP_SET_UAVS:
            return host.bindings.setUnorderedAccess(words, wordCount);
        case PG_ANDROID_OP_CLEAR_STATE:
            host.bindings.clear();
            return true;
        default:
            return false; // Unsupported opcodes are never silently treated as rendered.
    }
}


extern "C" bool PGAndroidGpuIsReady(void) {
    std::lock_guard<std::mutex> lock(host.mutex);
    return host.device != VK_NULL_HANDLE && host.resources != nullptr;
}
