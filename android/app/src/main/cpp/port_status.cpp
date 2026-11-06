#define VK_USE_PLATFORM_ANDROID_KHR
#include <jni.h>
#include <android/native_window_jni.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <string>
#include <vector>
#include <mutex>

namespace {
std::mutex gMutex;
VkInstance gInstance = VK_NULL_HANDLE;
VkSurfaceKHR gSurface = VK_NULL_HANDLE;
VkPhysicalDevice gPhysical = VK_NULL_HANDLE;
VkDevice gDevice = VK_NULL_HANDLE;
VkQueue gQueue = VK_NULL_HANDLE;
uint32_t gQueueFamily = 0;
VkSwapchainKHR gSwapchain = VK_NULL_HANDLE;
VkFormat gFormat = VK_FORMAT_UNDEFINED;
VkExtent2D gExtent{};
VkRenderPass gRenderPass = VK_NULL_HANDLE;
VkCommandPool gCommandPool = VK_NULL_HANDLE;
VkSemaphore gAcquire = VK_NULL_HANDLE;
std::vector<VkSemaphore> gComplete;
VkFence gFence = VK_NULL_HANDLE;
std::vector<VkImage> gImages;
std::vector<VkImageView> gViews;
std::vector<VkFramebuffer> gFramebuffers;
std::vector<VkCommandBuffer> gCommands;
ANativeWindow* gWindow = nullptr;
std::string gStatus = "Waiting for Android render surface";
uint64_t gFrames = 0;

void destroySwapchain() {
    if (gDevice == VK_NULL_HANDLE) return;
    vkDeviceWaitIdle(gDevice);
    for (auto f : gFramebuffers) vkDestroyFramebuffer(gDevice, f, nullptr);
    for (auto v : gViews) vkDestroyImageView(gDevice, v, nullptr);
    if (gRenderPass) vkDestroyRenderPass(gDevice, gRenderPass, nullptr);
    if (gCommandPool) vkDestroyCommandPool(gDevice, gCommandPool, nullptr);
    if (gAcquire) vkDestroySemaphore(gDevice, gAcquire, nullptr);
    for (auto semaphore : gComplete) if (semaphore) vkDestroySemaphore(gDevice, semaphore, nullptr);
    if (gFence) vkDestroyFence(gDevice, gFence, nullptr);
    if (gSwapchain) vkDestroySwapchainKHR(gDevice, gSwapchain, nullptr);
    gFramebuffers.clear(); gViews.clear(); gImages.clear(); gCommands.clear();
    gRenderPass = VK_NULL_HANDLE; gCommandPool = VK_NULL_HANDLE;
    gAcquire = VK_NULL_HANDLE; gComplete.clear(); gFence = VK_NULL_HANDLE;
    gSwapchain = VK_NULL_HANDLE;
}
void destroyAll() {
    if (gDevice) { destroySwapchain(); vkDestroyDevice(gDevice, nullptr); }
    if (gInstance) {
        if (gSurface) vkDestroySurfaceKHR(gInstance, gSurface, nullptr);
        vkDestroyInstance(gInstance, nullptr);
    }
    if (gWindow) ANativeWindow_release(gWindow);
    gWindow = nullptr; gSurface = VK_NULL_HANDLE; gInstance = VK_NULL_HANDLE;
    gDevice = VK_NULL_HANDLE; gPhysical = VK_NULL_HANDLE; gQueue = VK_NULL_HANDLE;
}
bool deviceExtension(VkPhysicalDevice d, const char* name) {
    uint32_t n = 0;
    if (vkEnumerateDeviceExtensionProperties(d, nullptr, &n, nullptr) != VK_SUCCESS || n == 0) return false;
    std::vector<VkExtensionProperties> e(n);
    if (vkEnumerateDeviceExtensionProperties(d, nullptr, &n, e.data()) != VK_SUCCESS) return false;
    return std::any_of(e.begin(), e.end(), [&](const auto& x) {
        return std::string(x.extensionName) == name;
    });
}
bool makeSwapchain() {
    if (!gDevice || !gWindow) return false;
    destroySwapchain();
    VkSurfaceCapabilitiesKHR caps{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gPhysical,gSurface,&caps)!=VK_SUCCESS) return false;
    uint32_t n = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(gPhysical, gSurface, &n, nullptr) != VK_SUCCESS || n == 0) return false;
    std::vector<VkSurfaceFormatKHR> formats(n);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(gPhysical, gSurface, &n, formats.data()) != VK_SUCCESS || n == 0) return false;
    VkSurfaceFormatKHR chosen=formats[0];
    // Some Android surface implementations report one undefined format to mean
    // that the application may choose any supported format.
    if (n == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        chosen.format = VK_FORMAT_R8G8B8A8_UNORM;
    } else {
        bool preferred = false;
        for (const auto& candidate : formats) {
            if (candidate.format == VK_FORMAT_R8G8B8A8_UNORM ||
                candidate.format == VK_FORMAT_B8G8R8A8_UNORM) {
                chosen = candidate;
                preferred = true;
                break;
            }
        }
        if (!preferred && chosen.format == VK_FORMAT_UNDEFINED) return false;
    }
    gFormat=chosen.format;
    VkExtent2D extent=caps.currentExtent;
    if (extent.width==UINT32_MAX) {
        extent.width=std::clamp((uint32_t)ANativeWindow_getWidth(gWindow),caps.minImageExtent.width,caps.maxImageExtent.width);
        extent.height=std::clamp((uint32_t)ANativeWindow_getHeight(gWindow),caps.minImageExtent.height,caps.maxImageExtent.height);
    }
    if (!extent.width || !extent.height) return false;
    uint32_t imageCount=caps.minImageCount+1;
    if (caps.maxImageCount && imageCount>caps.maxImageCount) imageCount=caps.maxImageCount;
    VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface=gSurface; sci.minImageCount=imageCount; sci.imageFormat=chosen.format; sci.imageColorSpace=chosen.colorSpace;
    sci.imageExtent=extent; sci.imageArrayLayers=1; sci.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE; sci.preTransform=caps.currentTransform;
    const VkCompositeAlphaFlagBitsKHR alphaModes[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };
    bool alphaSelected = false;
    for (auto mode : alphaModes) {
        if (caps.supportedCompositeAlpha & mode) {
            sci.compositeAlpha = mode;
            alphaSelected = true;
            break;
        }
    }
    if (!alphaSelected) return false;
    if ((caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0) return false;
    sci.presentMode=VK_PRESENT_MODE_FIFO_KHR; sci.clipped=VK_TRUE;
    if (vkCreateSwapchainKHR(gDevice,&sci,nullptr,&gSwapchain)!=VK_SUCCESS) return false;
    gExtent = extent;
    n = 0;
    if (vkGetSwapchainImagesKHR(gDevice, gSwapchain, &n, nullptr) != VK_SUCCESS || n == 0) return false;
    gImages.resize(n);
    if (vkGetSwapchainImagesKHR(gDevice, gSwapchain, &n, gImages.data()) != VK_SUCCESS || n == 0) return false;
    gImages.resize(n);
    VkAttachmentDescription attachment{}; attachment.format=gFormat; attachment.samples=VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR; attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE; attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED; attachment.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{}; sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS; sub.colorAttachmentCount=1; sub.pColorAttachments=&ref;
    VkSubpassDependency dep{}; dep.srcSubpass=VK_SUBPASS_EXTERNAL; dep.dstSubpass=0;
    dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dep.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount=1; rp.pAttachments=&attachment; rp.subpassCount=1; rp.pSubpasses=&sub; rp.dependencyCount=1; rp.pDependencies=&dep;
    if (vkCreateRenderPass(gDevice,&rp,nullptr,&gRenderPass)!=VK_SUCCESS) return false;
    VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; cp.queueFamilyIndex=gQueueFamily; cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(gDevice,&cp,nullptr,&gCommandPool)!=VK_SUCCESS) return false;
    gViews.resize(n); gFramebuffers.resize(n);
    for (uint32_t i=0;i<n;i++) {
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO}; vi.image=gImages[i]; vi.viewType=VK_IMAGE_VIEW_TYPE_2D; vi.format=gFormat;
        vi.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT; vi.subresourceRange.levelCount=1; vi.subresourceRange.layerCount=1;
        if (vkCreateImageView(gDevice,&vi,nullptr,&gViews[i])!=VK_SUCCESS) return false;
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO}; fb.renderPass=gRenderPass; fb.attachmentCount=1; fb.pAttachments=&gViews[i]; fb.width=extent.width; fb.height=extent.height; fb.layers=1;
        if (vkCreateFramebuffer(gDevice,&fb,nullptr,&gFramebuffers[i])!=VK_SUCCESS) return false;
    }
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO}; ca.commandPool=gCommandPool; ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY; ca.commandBufferCount=n;
    gCommands.resize(n); if (vkAllocateCommandBuffers(gDevice,&ca,gCommands.data())!=VK_SUCCESS) return false;
    VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(gDevice,&sem,nullptr,&gAcquire)!=VK_SUCCESS) return false;
    gComplete.resize(n, VK_NULL_HANDLE);
    for (auto &semaphore : gComplete) {
        if (vkCreateSemaphore(gDevice,&sem,nullptr,&semaphore)!=VK_SUCCESS) return false;
    }
    VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}; fc.flags=VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(gDevice,&fc,nullptr,&gFence)!=VK_SUCCESS) return false;
    return true;
}
}
extern "C" JNIEXPORT jstring JNICALL Java_io_github_ravi9733_gta5port_MainActivity_nativeVulkanStatus(JNIEnv* e,jobject){std::lock_guard<std::mutex> l(gMutex);return e->NewStringUTF(gStatus.c_str());}
extern "C" JNIEXPORT jstring JNICALL Java_io_github_ravi9733_gta5port_MainActivity_nativeAttachSurface(JNIEnv* env,jobject,jobject surface){
 std::lock_guard<std::mutex> l(gMutex); destroyAll(); gWindow=ANativeWindow_fromSurface(env,surface);
 if(!gWindow){gStatus="Android native window unavailable";return env->NewStringUTF(gStatus.c_str());}
 const char* exts[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
 VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="GTA V Android";app.apiVersion=VK_API_VERSION_1_1;
 VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&app;ci.enabledExtensionCount=2;ci.ppEnabledExtensionNames=exts;
 VkResult r=vkCreateInstance(&ci,nullptr,&gInstance);
 if(r!=VK_SUCCESS){gStatus="vkCreateInstance failed: "+std::to_string(r);destroyAll();return env->NewStringUTF(gStatus.c_str());}
 VkAndroidSurfaceCreateInfoKHR si{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};si.window=gWindow;
 r=vkCreateAndroidSurfaceKHR(gInstance,&si,nullptr,&gSurface);
 if(r!=VK_SUCCESS){gStatus="Android surface creation failed: "+std::to_string(r);destroyAll();return env->NewStringUTF(gStatus.c_str());}
 uint32_t count=0;
 r=vkEnumeratePhysicalDevices(gInstance,&count,nullptr);
 if(r!=VK_SUCCESS||count==0){gStatus="No Vulkan physical devices found: "+std::to_string(r);destroyAll();return env->NewStringUTF(gStatus.c_str());}
 std::vector<VkPhysicalDevice> ds(count);
 r=vkEnumeratePhysicalDevices(gInstance,&count,ds.data());
 if(r!=VK_SUCCESS||count==0){gStatus="Vulkan physical-device enumeration failed: "+std::to_string(r);destroyAll();return env->NewStringUTF(gStatus.c_str());}
 for(auto d:ds){uint32_t qn=0;vkGetPhysicalDeviceQueueFamilyProperties(d,&qn,nullptr);std::vector<VkQueueFamilyProperties> qs(qn);vkGetPhysicalDeviceQueueFamilyProperties(d,&qn,qs.data());
  for(uint32_t q=0;q<qn;q++){VkBool32 present=VK_FALSE;
   if(vkGetPhysicalDeviceSurfaceSupportKHR(d,q,gSurface,&present)!=VK_SUCCESS)continue;
   if((qs[q].queueFlags&VK_QUEUE_GRAPHICS_BIT)&&present&&deviceExtension(d,VK_KHR_SWAPCHAIN_EXTENSION_NAME)){gPhysical=d;gQueueFamily=q;break;}}
  if(gPhysical)break;
 }
 if(!gPhysical){gStatus="No Vulkan graphics/present queue with swapchain support";destroyAll();return env->NewStringUTF(gStatus.c_str());}
 float priority=1.f;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=gQueueFamily;qi.queueCount=1;qi.pQueuePriorities=&priority;
 const char* dext[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME};VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;di.enabledExtensionCount=1;di.ppEnabledExtensionNames=dext;
 r=vkCreateDevice(gPhysical,&di,nullptr,&gDevice);if(r!=VK_SUCCESS){gStatus="vkCreateDevice failed: "+std::to_string(r);destroyAll();return env->NewStringUTF(gStatus.c_str());}
 vkGetDeviceQueue(gDevice,gQueueFamily,0,&gQueue);
 if(!makeSwapchain()){gStatus="Vulkan device ready, swapchain setup failed";destroyAll();return env->NewStringUTF(gStatus.c_str());}
 VkPhysicalDeviceProperties p{};vkGetPhysicalDeviceProperties(gPhysical,&p);
 gStatus="Vulkan swapchain ready: "+std::string(p.deviceName);gFrames=0;return env->NewStringUTF(gStatus.c_str());
}
extern "C" JNIEXPORT void JNICALL Java_io_github_ravi9733_gta5port_MainActivity_nativeResizeSurface(JNIEnv*,jobject,jint width,jint height){std::lock_guard<std::mutex> l(gMutex);if(!gDevice||!gWindow)return;if(width<=0||height<=0){gStatus="Waiting for a non-zero surface size";return;}if(makeSwapchain()){gStatus="Vulkan swapchain resized";}else{destroySwapchain();gStatus="Vulkan swapchain resize failed; waiting for surface recovery";}}
extern "C" JNIEXPORT void JNICALL Java_io_github_ravi9733_gta5port_MainActivity_nativeRenderFrame(JNIEnv*,jobject){
 std::lock_guard<std::mutex> l(gMutex);if(!gDevice||gCommands.empty())return;
 if(!gFence||vkWaitForFences(gDevice,1,&gFence,VK_TRUE,UINT64_MAX)!=VK_SUCCESS)return;
 uint32_t index=0;VkResult r=vkAcquireNextImageKHR(gDevice,gSwapchain,UINT64_MAX,gAcquire,VK_NULL_HANDLE,&index);
 auto recoverSwapchain=[&](const char* reason){if(!makeSwapchain()){destroySwapchain();gStatus=std::string(reason)+"; swapchain rebuild failed";}else{gStatus=std::string(reason)+"; swapchain rebuilt";}};
 if(r==VK_ERROR_OUT_OF_DATE_KHR){recoverSwapchain("Vulkan surface out of date");return;}if(r!=VK_SUCCESS&&r!=VK_SUBOPTIMAL_KHR){gStatus="Vulkan image acquire failed: "+std::to_string(r);return;}
 auto recoverAcquiredImage=[&](){recoverSwapchain("Vulkan frame recovery");};
 VkCommandBuffer cmd=gCommands[index];if(vkResetCommandBuffer(cmd,0)!=VK_SUCCESS){recoverAcquiredImage();return;}
 VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};if(vkBeginCommandBuffer(cmd,&bi)!=VK_SUCCESS){recoverAcquiredImage();return;}
 VkClearValue clear{};float pulse=(float)((gFrames++%240)/240.0);clear.color.float32[0]=0.025f+0.025f*pulse;clear.color.float32[1]=0.045f;clear.color.float32[2]=0.075f;clear.color.float32[3]=1.f;
 VkRenderPassBeginInfo ri{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};ri.renderPass=gRenderPass;ri.framebuffer=gFramebuffers[index];ri.renderArea.extent=gExtent;ri.clearValueCount=1;ri.pClearValues=&clear;
 vkCmdBeginRenderPass(cmd,&ri,VK_SUBPASS_CONTENTS_INLINE);vkCmdEndRenderPass(cmd);if(vkEndCommandBuffer(cmd)!=VK_SUCCESS){recoverAcquiredImage();return;}
 VkPipelineStageFlags waitStage=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
 submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&gAcquire;submit.pWaitDstStageMask=&waitStage;submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&gComplete[index];
 if(vkResetFences(gDevice,1,&gFence)!=VK_SUCCESS){recoverAcquiredImage();return;}
 if(vkQueueSubmit(gQueue,1,&submit,gFence)!=VK_SUCCESS){
  // The acquire semaphore was not consumed by a successful queue submission.
  // Rebuild the swapchain so neither that semaphore nor the acquired image is reused.
  recoverAcquiredImage();
  return;
 }
 VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&gComplete[index];present.swapchainCount=1;present.pSwapchains=&gSwapchain;present.pImageIndices=&index;
 r=vkQueuePresentKHR(gQueue,&present);
 if(r!=VK_SUCCESS&&r!=VK_SUBOPTIMAL_KHR&&r!=VK_ERROR_OUT_OF_DATE_KHR){
  gStatus="Vulkan present failed: "+std::to_string(r);recoverAcquiredImage();return;
 }
 // The render-finished semaphore belongs to this swapchain image. It is not reused
 // until that image is acquired again, avoiding a queue-wide stall on every frame.
 if(r==VK_ERROR_OUT_OF_DATE_KHR||r==VK_SUBOPTIMAL_KHR)makeSwapchain();
}
extern "C" JNIEXPORT void JNICALL Java_io_github_ravi9733_gta5port_MainActivity_nativeDetachSurface(JNIEnv*,jobject){std::lock_guard<std::mutex> l(gMutex);destroyAll();gStatus="Render surface detached";}
