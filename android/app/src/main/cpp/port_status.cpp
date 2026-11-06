#define VK_USE_PLATFORM_ANDROID_KHR
#include <jni.h>
#include <android/native_window_jni.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <mutex>

namespace {
std::mutex gMutex;
VkInstance gInstance = VK_NULL_HANDLE;
VkSurfaceKHR gSurface = VK_NULL_HANDLE;
std::string gStatus = "Waiting for Android render surface";

void destroyVulkan() {
    if (gInstance != VK_NULL_HANDLE) {
        if (gSurface != VK_NULL_HANDLE) vkDestroySurfaceKHR(gInstance, gSurface, nullptr);
        vkDestroyInstance(gInstance, nullptr);
    }
    gSurface = VK_NULL_HANDLE;
    gInstance = VK_NULL_HANDLE;
}

bool hasDeviceExtension(VkPhysicalDevice device, const char* wanted) {
    uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS) return false;
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS) return false;
    for (const auto& extension : extensions) if (std::string(extension.extensionName) == wanted) return true;
    return false;
}
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_github_ravi9733_gta5port_MainActivity_nativeVulkanStatus(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    return env->NewStringUTF(gStatus.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_github_ravi9733_gta5port_MainActivity_nativeAttachSurface(JNIEnv* env, jobject, jobject surface) {
    std::lock_guard<std::mutex> lock(gMutex);
    destroyVulkan();

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        gStatus = "Could not obtain Android native window";
        return env->NewStringUTF(gStatus.c_str());
    }

    const char* extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME };
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "GTA V Android Port";
    app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app.pEngineName = "GTA V Port";
    app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create.pApplicationInfo = &app;
    create.enabledExtensionCount = 2;
    create.ppEnabledExtensionNames = extensions;

    VkResult result = vkCreateInstance(&create, nullptr, &gInstance);
    if (result != VK_SUCCESS) {
        gStatus = "Vulkan instance creation failed: " + std::to_string(result);
        ANativeWindow_release(window);
        return env->NewStringUTF(gStatus.c_str());
    }

    VkAndroidSurfaceCreateInfoKHR surfaceInfo{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    surfaceInfo.window = window;
    result = vkCreateAndroidSurfaceKHR(gInstance, &surfaceInfo, nullptr, &gSurface);
    ANativeWindow_release(window);
    if (result != VK_SUCCESS) {
        gStatus = "Vulkan Android surface creation failed: " + std::to_string(result);
        destroyVulkan();
        return env->NewStringUTF(gStatus.c_str());
    }

    uint32_t count = 0;
    result = vkEnumeratePhysicalDevices(gInstance, &count, nullptr);
    if (result != VK_SUCCESS || count == 0) {
        gStatus = "No Vulkan physical device found";
        destroyVulkan();
        return env->NewStringUTF(gStatus.c_str());
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(gInstance, &count, devices.data());

    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device, &properties);
        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queues(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queues.data());
        for (uint32_t q = 0; q < queueCount; ++q) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, q, gSurface, &present);
            if ((queues[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present &&
                hasDeviceExtension(device, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
                gStatus = "Vulkan ready: " + std::string(properties.deviceName) +
                          " (graphics + present queue, swapchain supported)";
                return env->NewStringUTF(gStatus.c_str());
            }
        }
    }

    gStatus = "Vulkan device found, but no graphics/present queue with swapchain support";
    destroyVulkan();
    return env->NewStringUTF(gStatus.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_ravi9733_gta5port_MainActivity_nativeDetachSurface(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(gMutex);
    destroyVulkan();
    gStatus = "Render surface detached";
}
