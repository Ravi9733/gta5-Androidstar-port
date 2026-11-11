#define VK_USE_PLATFORM_ANDROID_KHR
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <vector>

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
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamily = UINT32_MAX;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
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
    if (host.device && host.swapchain) vkDestroySwapchainKHR(host.device, host.swapchain, nullptr);
    host.swapchain = VK_NULL_HANDLE;
}

void destroyLocked() {
    destroySwapchain();
    if (host.device) vkDestroyDevice(host.device, nullptr);
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
    LOGI("Vulkan swapchain ready: %ux%u, format %d", extent.width, extent.height,
         static_cast<int>(selected.format));
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
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    if (!check(vkCreateDevice(host.physical, &deviceInfo, nullptr, &host.device), "vkCreateDevice")) return false;
    vkGetDeviceQueue(host.device, host.queueFamily, 0, &host.queue);

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
Java_com_ravi_gta5port_MainActivity_nativeResize(JNIEnv *, jclass, jint width, jint height) {
    std::lock_guard<std::mutex> lock(host.mutex);
    if (width <= 0 || height <= 0) return JNI_FALSE;
    return makeSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height)) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_ravi_gta5port_MainActivity_nativeDestroy(JNIEnv *, jclass) {
    std::lock_guard<std::mutex> lock(host.mutex);
    destroyLocked();
}
