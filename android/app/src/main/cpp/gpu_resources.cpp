#include "gpu_resources.h"
#include "BCTextureDecoder.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <limits>
#include <vector>

namespace {
constexpr uint32_t kBindVertex = 0x01;
constexpr uint32_t kBindIndex = 0x02;
constexpr uint32_t kBindConstant = 0x04;
constexpr uint32_t kBindShaderResource = 0x08;
constexpr uint32_t kBindUnorderedAccess = 0x80;
constexpr uint32_t kMaxBufferBytes = 512u * 1024u * 1024u;
}


namespace {
VkFormat dxgiToVkFormat(uint32_t dxgi) {
    switch (dxgi) {
        case 2: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case 9: case 10: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case 16: return VK_FORMAT_R32G32_SFLOAT;
        case 27: case 28: return VK_FORMAT_R8G8B8A8_UNORM;
        case 29: return VK_FORMAT_R8G8B8A8_SRGB;
        case 33: case 34: return VK_FORMAT_R16G16_SFLOAT;
        case 40: return VK_FORMAT_D32_SFLOAT;
        case 41: return VK_FORMAT_R32_SFLOAT;
        case 44: case 45: case 21: case 22: case 46: case 47: return VK_FORMAT_D24_UNORM_S8_UINT;
        case 48: case 49: return VK_FORMAT_R8G8_UNORM;
        case 54: return VK_FORMAT_R16_SFLOAT;
        case 55: return VK_FORMAT_D16_UNORM;
        case 60: case 61: case 65: return VK_FORMAT_R8_UNORM;
        case 70: case 71: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case 72: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case 73: case 74: return VK_FORMAT_BC2_UNORM_BLOCK;
        case 75: return VK_FORMAT_BC2_SRGB_BLOCK;
        case 76: case 77: return VK_FORMAT_BC3_UNORM_BLOCK;
        case 78: return VK_FORMAT_BC3_SRGB_BLOCK;
        case 79: case 80: return VK_FORMAT_BC4_UNORM_BLOCK;
        case 81: return VK_FORMAT_BC4_SNORM_BLOCK;
        case 82: case 83: return VK_FORMAT_BC5_UNORM_BLOCK;
        case 84: return VK_FORMAT_BC5_SNORM_BLOCK;
        case 87: return VK_FORMAT_B8G8R8A8_UNORM;
        case 91: return VK_FORMAT_B8G8R8A8_SRGB;
        case 94: case 95: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case 96: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
        case 97: case 98: return VK_FORMAT_BC7_UNORM_BLOCK;
        case 99: return VK_FORMAT_BC7_SRGB_BLOCK;
        default: return VK_FORMAT_UNDEFINED;
    }
}


uint32_t bcDecoderFormat(uint32_t dxgi) {
    switch (dxgi) {
        case 70: case 71: case 72: return 1;
        case 73: case 74: case 75: return 2;
        case 76: case 77: case 78: return 3;
        case 79: case 80: return 4;
        case 82: case 83: return 5;
        case 94: case 95: return 6;
        case 97: case 98: case 99: return 7;
        default: return 0; // Signed BC4/5 and signed BC6 require dedicated decoders.
    }
}
VkFormat decodedBCFormat(uint32_t bc, uint32_t dxgi) {
    const bool srgb = dxgi == 72 || dxgi == 75 || dxgi == 78 || dxgi == 99;
    switch (bc) {
        case 1: case 2: case 3: case 7:
            return srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        case 4: return VK_FORMAT_R8_UNORM;
        case 5: return VK_FORMAT_R8G8_UNORM;
        case 6: return VK_FORMAT_R16G16B16A16_SFLOAT;
        default: return VK_FORMAT_UNDEFINED;
    }
}

struct FormatBlock {
    uint32_t width;
    uint32_t height;
    uint32_t bytes;
};
bool formatBlock(VkFormat format, FormatBlock &block) {
    switch (format) {
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
            block = {4, 4, 8}; return true;
        case VK_FORMAT_BC2_UNORM_BLOCK:
        case VK_FORMAT_BC2_SRGB_BLOCK:
        case VK_FORMAT_BC3_UNORM_BLOCK:
        case VK_FORMAT_BC3_SRGB_BLOCK:
        case VK_FORMAT_BC5_UNORM_BLOCK:
        case VK_FORMAT_BC5_SNORM_BLOCK:
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        case VK_FORMAT_BC7_UNORM_BLOCK:
        case VK_FORMAT_BC7_SRGB_BLOCK:
            block = {4, 4, 16}; return true;
        case VK_FORMAT_R32G32B32A32_SFLOAT: block = {1,1,16}; return true;
        case VK_FORMAT_R16G16B16A16_SFLOAT: block = {1,1,8}; return true;
        case VK_FORMAT_R32G32_SFLOAT: block = {1,1,8}; return true;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            block = {1,1,4}; return true;
        case VK_FORMAT_R16G16_SFLOAT: block = {1,1,4}; return true;
        case VK_FORMAT_R8G8_UNORM: block = {1,1,2}; return true;
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT: block = {1,1,4}; return true;
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_D16_UNORM: block = {1,1,2}; return true;
        case VK_FORMAT_R8_UNORM: block = {1,1,1}; return true;
        default: return false;
    }
}

bool isDepthVkFormat(VkFormat format) {
    return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D24_UNORM_S8_UINT ||
           format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}
}

PGAndroidGpuResources::PGAndroidGpuResources(VkPhysicalDevice physical, VkDevice device,
                                                   VkQueue queue, uint32_t queueFamily,
                                                   bool textureCompressionBC)
    : physical_(physical), device_(device), queue_(queue), queueFamily_(queueFamily),
      textureCompressionBC_(textureCompressionBC) {}

PGAndroidGpuResources::~PGAndroidGpuResources() { reset(); }

uint32_t PGAndroidGpuResources::findMemoryType(
    uint32_t bits, VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred) const {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical_, &properties);
    uint32_t fallback = UINT32_MAX;
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if (!(bits & (1u << i))) continue;
        const auto flags = properties.memoryTypes[i].propertyFlags;
        if ((flags & required) != required) continue;
        if ((flags & preferred) == preferred) return i;
        if (fallback == UINT32_MAX) fallback = i;
    }
    return fallback;
}

bool PGAndroidGpuResources::createBuffer(const uint32_t *p, uint32_t n) {
    if (!p || n < 5) return false;
    const uint32_t id = p[0], bytes = p[1], bind = p[2];
    if (!id || !bytes || bytes > kMaxBufferBytes || buffers_.count(id) || textures_.count(id) || views_.count(id)) return false;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (bind & kBindVertex) usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (bind & kBindIndex) usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (bind & kBindConstant) usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (bind & (kBindShaderResource | kBindUnorderedAccess))
        usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    BufferResource resource{};
    resource.size = bytes;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = bytes;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &info, nullptr, &resource.buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, resource.buffer, &requirements);
    const uint32_t memoryType = findMemoryType(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == UINT32_MAX) {
        vkDestroyBuffer(device_, resource.buffer, nullptr);
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physical_, &memoryProperties);
    resource.memoryFlags = memoryProperties.memoryTypes[memoryType].propertyFlags;
    if (vkAllocateMemory(device_, &allocation, nullptr, &resource.memory) != VK_SUCCESS ||
        vkBindBufferMemory(device_, resource.buffer, resource.memory, 0) != VK_SUCCESS ||
        vkMapMemory(device_, resource.memory, 0, VK_WHOLE_SIZE, 0, &resource.mapped) != VK_SUCCESS) {
        vkDestroyBuffer(device_, resource.buffer, nullptr);
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
        return false;
    }
    std::memset(resource.mapped, 0, bytes);
    buffers_.emplace(id, resource);
    return true;
}


bool PGAndroidGpuResources::createTexture(const uint32_t *p, uint32_t n) {
    if (!p || n < 10) return false;
    const uint32_t id=p[0], dim=p[1], width=p[2], height=p[3], depth=p[4];
    const uint32_t mipLevels=p[5], layers=p[6], dxgi=p[7], bind=p[8], samples=p[9];
    if (!id || !width || !height || !depth || !mipLevels || !layers ||
        dim < 1 || dim > 3 || samples == 0 || (samples & (samples-1)) ||
        textures_.count(id) || buffers_.count(id) || views_.count(id)) return false;
    const uint32_t maxDimension = std::max(width, std::max(height, depth));
    uint32_t maxMips=1;
    for (uint32_t d=maxDimension; d>1; d>>=1) ++maxMips;
    if (mipLevels > maxMips || (dim == 1 && height != 1) ||
        (dim == 1 && depth != 1) || (dim == 2 && depth != 1) ||
        (dim == 3 && layers != 1) || (samples > 1 && (mipLevels != 1 || dim != 2)))
        return false;

    VkFormat format=dxgiToVkFormat(dxgi);
    if (format == VK_FORMAT_UNDEFINED) return false;
    VkImageUsageFlags usage=VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (bind & kBindShaderResource) usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (bind & 0x20u) usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (bind & 0x40u) usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (bind & kBindUnorderedAccess) usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    VkFormatFeatureFlags required=0;
    if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) required |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) required |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) required |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (usage & VK_IMAGE_USAGE_STORAGE_BIT) required |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(physical_, format, &formatProperties);
    FormatBlock sourceBlock{};
    const bool compressedFormat = formatBlock(format, sourceBlock) && sourceBlock.width == 4 && sourceBlock.height == 4;
    bool decodeBC=false;
    if (compressedFormat && !textureCompressionBC_) formatProperties.optimalTilingFeatures = 0;
    const uint32_t bcFormat=bcDecoderFormat(dxgi);
    if ((formatProperties.optimalTilingFeatures & required) != required) {
        if (!bcFormat) return false;
        format=decodedBCFormat(bcFormat, dxgi);
        if (format == VK_FORMAT_UNDEFINED) return false;
        vkGetPhysicalDeviceFormatProperties(physical_, format, &formatProperties);
        if ((formatProperties.optimalTilingFeatures & required) != required) return false;
        decodeBC=true;
    }

    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.flags = (dim == 2 && layers >= 6 && (layers % 6u) == 0) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    info.imageType = dim == 1 ? VK_IMAGE_TYPE_1D : dim == 3 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    info.format=format;
    info.extent={width, dim == 1 ? 1u : height, dim == 3 ? depth : 1u};
    info.mipLevels=mipLevels;
    info.arrayLayers=dim == 3 ? 1u : layers;
    info.samples=static_cast<VkSampleCountFlagBits>(samples);
    info.tiling=VK_IMAGE_TILING_OPTIMAL;
    info.usage=usage;
    info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;

    TextureResource resource{};
    resource.width=width; resource.height=height; resource.depth=depth;
    resource.mipLevels=mipLevels; resource.layers=layers; resource.dimension=dim;
    resource.bind=bind; resource.format=format;
    resource.bcFormat=bcFormat; resource.decodeBC=decodeBC; resource.sourceFormat=dxgi;
    if (vkCreateImage(device_, &info, nullptr, &resource.image) != VK_SUCCESS) return false;
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, resource.image, &requirements);
    const uint32_t memoryType=findMemoryType(requirements.memoryTypeBits, 0,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryType == UINT32_MAX) {
        vkDestroyImage(device_, resource.image, nullptr);
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize=requirements.size;
    allocation.memoryTypeIndex=memoryType;
    if (vkAllocateMemory(device_, &allocation, nullptr, &resource.memory) != VK_SUCCESS ||
        vkBindImageMemory(device_, resource.image, resource.memory, 0) != VK_SUCCESS) {
        vkDestroyImage(device_, resource.image, nullptr);
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
        return false;
    }
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image=resource.image;
    viewInfo.viewType=dim == 1 ? (layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D) :
                      dim == 3 ? VK_IMAGE_VIEW_TYPE_3D :
                      layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format=format;
    viewInfo.subresourceRange.aspectMask=isDepthVkFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    viewInfo.subresourceRange.baseMipLevel=0;
    viewInfo.subresourceRange.levelCount=mipLevels;
    viewInfo.subresourceRange.baseArrayLayer=0;
    viewInfo.subresourceRange.layerCount=dim == 3 ? 1u : layers;
    if (vkCreateImageView(device_, &viewInfo, nullptr, &resource.view) != VK_SUCCESS) {
        vkDestroyImage(device_, resource.image, nullptr);
        vkFreeMemory(device_, resource.memory, nullptr);
        return false;
    }
    textures_.emplace(id, resource);
    return true;
}

bool PGAndroidGpuResources::uploadBuffer(const uint32_t *p, uint32_t n) {
    if (!p || n < 3) return false;
    const uint32_t id = p[0], offset = p[1], bytes = p[2];
    auto it = buffers_.find(id);
    if (it == buffers_.end() || offset > it->second.size ||
        bytes > it->second.size - offset || static_cast<uint64_t>(bytes) > static_cast<uint64_t>(n - 3u) * sizeof(uint32_t))
        return false;
    if (bytes) {
        std::memcpy(static_cast<uint8_t *>(it->second.mapped) + offset, p + 3, bytes);
        if (!(it->second.memoryFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
            range.memory = it->second.memory;
            range.offset = 0;
            range.size = VK_WHOLE_SIZE;
            if (vkFlushMappedMemoryRanges(device_, 1, &range) != VK_SUCCESS) return false;
        }
    }
    return true;
}


bool PGAndroidGpuResources::uploadTexture(const uint32_t *p, uint32_t n) {
    if (!p || n < 12) return false;
    const uint32_t id=p[0], mip=p[1], slice=p[2], x=p[3], y=p[4], z=p[5];
    const uint32_t width=p[6], height=p[7], depth=p[8];
    uint32_t rowPitch=p[9], rowsPerImage=p[10];
    const uint32_t dataBytes=p[11];
    auto found=textures_.find(id);
    if (found == textures_.end()) return false;
    TextureResource &texture=found->second;
    if (!dataBytes || static_cast<uint64_t>(dataBytes) >
        static_cast<uint64_t>(n-12u)*sizeof(uint32_t) || mip >= texture.mipLevels ||
        !width || !height || !depth) return false;

    const uint32_t mipWidth=std::max(1u, texture.width >> mip);
    const uint32_t mipHeight=texture.dimension == 1 ? 1u : std::max(1u, texture.height >> mip);
    const uint32_t mipDepth=texture.dimension == 3 ? std::max(1u, texture.depth >> mip) : 1u;
    if (x > mipWidth || width > mipWidth-x || y > mipHeight || height > mipHeight-y ||
        z > mipDepth || depth > mipDepth-z) return false;
    if (texture.dimension == 1 && (y || z || height != 1 || depth != 1)) return false;
    if (texture.dimension == 2 && (z || depth > texture.layers ||
        slice > texture.layers || depth > texture.layers-slice)) return false;
    if (texture.dimension == 3 && (slice || depth > mipDepth-z)) return false;

    const uint8_t *uploadData=reinterpret_cast<const uint8_t *>(p+12);
    uint32_t uploadBytes=dataBytes;
    std::unique_ptr<void, decltype(&std::free)> decoded(nullptr, &std::free);
    if (texture.decodeBC) {
        if ((x & 3u) || (y & 3u)) return false;
        size_t decodedSize=0;
        decoded.reset(PGDecodeBC(static_cast<int>(texture.bcFormat), width, height, depth,
                                 rowPitch, rowsPerImage, uploadData, dataBytes, &decodedSize));
        if (!decoded || decodedSize > UINT32_MAX) return false;
        uploadData=static_cast<const uint8_t *>(decoded.get());
        uploadBytes=static_cast<uint32_t>(decodedSize);
        rowPitch=0;
        rowsPerImage=0;
    }

    FormatBlock block{};
    if (!formatBlock(texture.format, block)) return false;
    if ((block.width > 1 && ((x % block.width) || (width % block.width && x+width != mipWidth))) ||
        (block.height > 1 && ((y % block.height) || (height % block.height && y+height != mipHeight))))
        return false;
    const uint32_t blockColumns=(width+block.width-1)/block.width;
    const uint32_t blockRows=(height+block.height-1)/block.height;
    const uint64_t tightRow=static_cast<uint64_t>(blockColumns)*block.bytes;
    const uint32_t actualRowPitch=rowPitch ? rowPitch : static_cast<uint32_t>(tightRow);
    const uint32_t actualRows=rowsPerImage ? rowsPerImage : blockRows;
    if (actualRowPitch < tightRow || actualRowPitch % block.bytes ||
        actualRows < blockRows) return false;
    const uint64_t imageBytes=static_cast<uint64_t>(actualRowPitch)*actualRows;
    const uint64_t requiredBytes=imageBytes*(depth-1u)+
        static_cast<uint64_t>(actualRowPitch)*blockRows;
    if (requiredBytes > uploadBytes || requiredBytes > SIZE_MAX) return false;

    VkBuffer staging=VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory=VK_NULL_HANDLE;
    void *mapped=nullptr;
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size=uploadBytes;
    bufferInfo.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &staging) != VK_SUCCESS) return false;
    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device_, staging, &memoryRequirements);
    const uint32_t memoryType=findMemoryType(memoryRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == UINT32_MAX) { vkDestroyBuffer(device_, staging, nullptr); return false; }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize=memoryRequirements.size;
    allocation.memoryTypeIndex=memoryType;
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physical_, &memoryProperties);
    const VkMemoryPropertyFlags memoryFlags=memoryProperties.memoryTypes[memoryType].propertyFlags;
    if (vkAllocateMemory(device_, &allocation, nullptr, &stagingMemory) != VK_SUCCESS ||
        vkBindBufferMemory(device_, staging, stagingMemory, 0) != VK_SUCCESS ||
        vkMapMemory(device_, stagingMemory, 0, VK_WHOLE_SIZE, 0, &mapped) != VK_SUCCESS) {
        vkDestroyBuffer(device_, staging, nullptr);
        if (stagingMemory) vkFreeMemory(device_, stagingMemory, nullptr);
        return false;
    }
    std::memcpy(mapped, uploadData, uploadBytes);
    if (!(memoryFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        range.memory=stagingMemory; range.offset=0; range.size=VK_WHOLE_SIZE;
        if (vkFlushMappedMemoryRanges(device_, 1, &range) != VK_SUCCESS) {
            vkUnmapMemory(device_, stagingMemory);
            vkDestroyBuffer(device_, staging, nullptr);
            vkFreeMemory(device_, stagingMemory, nullptr);
            return false;
        }
    }

    VkCommandPool pool=VK_NULL_HANDLE;
    VkCommandBuffer command=VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags=VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex=queueFamily_;
    bool ok=vkCreateCommandPool(device_, &poolInfo, nullptr, &pool) == VK_SUCCESS;
    if (ok) {
        VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandInfo.commandPool=pool; commandInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandInfo.commandBufferCount=1;
        ok=vkAllocateCommandBuffers(device_, &commandInfo, &command) == VK_SUCCESS;
    }
    if (ok) {
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        ok=vkBeginCommandBuffer(command, &begin) == VK_SUCCESS;
    }
    if (ok) {
        VkImageMemoryBarrier toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toTransfer.srcAccessMask=texture.layout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 :
            (texture.layout == VK_IMAGE_LAYOUT_GENERAL ? VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT :
             VK_ACCESS_SHADER_READ_BIT);
        toTransfer.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransfer.oldLayout=texture.layout;
        toTransfer.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image=texture.image;
        toTransfer.subresourceRange.aspectMask=isDepthVkFormat(texture.format) ?
            VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        if (texture.format == VK_FORMAT_D24_UNORM_S8_UINT ||
            texture.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
            toTransfer.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        toTransfer.subresourceRange.baseMipLevel=0;
        toTransfer.subresourceRange.levelCount=texture.mipLevels;
        toTransfer.subresourceRange.baseArrayLayer=0;
        toTransfer.subresourceRange.layerCount=texture.dimension == 3 ? 1u : texture.layers;
        vkCmdPipelineBarrier(command,
            texture.layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT :
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

        VkBufferImageCopy copy{};
        copy.bufferOffset=0;
        copy.bufferRowLength=actualRowPitch == tightRow ? 0 :
            (actualRowPitch / block.bytes) * block.width;
        copy.bufferImageHeight=rowsPerImage ? actualRows*block.height : 0;
        copy.imageSubresource.aspectMask=toTransfer.subresourceRange.aspectMask;
        copy.imageSubresource.mipLevel=mip;
        copy.imageSubresource.baseArrayLayer=texture.dimension == 3 ? 0u : slice;
        copy.imageSubresource.layerCount=texture.dimension == 3 ? 1u : depth;
        copy.imageOffset={static_cast<int32_t>(x),static_cast<int32_t>(y),
                          static_cast<int32_t>(texture.dimension == 3 ? z : 0u)};
        copy.imageExtent={width,height,texture.dimension == 3 ? depth : 1u};
        vkCmdCopyBufferToImage(command, staging, texture.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        const VkImageLayout finalLayout=(texture.bind & kBindShaderResource) ?
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
        VkImageMemoryBarrier toFinal=toTransfer;
        toFinal.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        toFinal.dstAccessMask=finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ?
            VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        toFinal.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toFinal.newLayout=finalLayout;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &toFinal);
        ok=vkEndCommandBuffer(command) == VK_SUCCESS;
        if (ok) {
            VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit.commandBufferCount=1; submit.pCommandBuffers=&command;
            ok=vkQueueSubmit(queue_, 1, &submit, VK_NULL_HANDLE) == VK_SUCCESS;
            if (ok) ok=vkQueueWaitIdle(queue_) == VK_SUCCESS;
            if (ok) texture.layout=finalLayout;
        }
    }
    if (pool) vkDestroyCommandPool(device_, pool, nullptr);
    vkUnmapMemory(device_, stagingMemory);
    vkDestroyBuffer(device_, staging, nullptr);
    vkFreeMemory(device_, stagingMemory, nullptr);
    return ok;
}


bool PGAndroidGpuResources::createTextureView(const uint32_t *p, uint32_t n, bool unorderedAccess) {
    // Original iOS createView packet: id, isBuffer, resourceId, DXGI view format,
    // D3D view dimension, baseMip, mipCount, firstSlice, (SRV sliceCount | UAV flags).
    if (!p || n < 9 || p[0] == 0 || p[1] != 0) return false;
    const uint32_t id=p[0], resourceId=p[2], viewFormat=p[3], dimension=p[4];
    const uint32_t baseMip=p[5], requestedMips=p[6], firstLayer=p[7], finalWord=p[8];
    if (views_.count(id) || buffers_.count(id) || textures_.count(id)) return false;
    auto found=textures_.find(resourceId);
    if (found == textures_.end()) return false;
    const TextureResource &texture=found->second;
    if (unorderedAccess && !(texture.bind & kBindUnorderedAccess)) return false;
    if (!unorderedAccess && !(texture.bind & kBindShaderResource)) return false;
    if (baseMip >= texture.mipLevels) return false;
    const uint32_t mipCount=(requestedMips == 0 || requestedMips == 0xffffffffu)
        ? texture.mipLevels-baseMip : std::min(requestedMips, texture.mipLevels-baseMip);
    if (!mipCount) return false;

    VkImageViewType type=VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    uint32_t baseLayer=firstLayer, layerCount=1;
    switch (dimension) {
        case 0: // D1
            if (texture.dimension != 1) return false;
            type=texture.layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            layerCount=texture.layers > 1 ? texture.layers-firstLayer : 1;
            break;
        case 1: // D2
            if (texture.dimension != 2 || texture.layers != 1) return false;
            type=VK_IMAGE_VIEW_TYPE_2D;
            break;
        case 2: // D2Array
            if (texture.dimension != 2) return false;
            type=VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            layerCount=(!unorderedAccess && finalWord) ? std::min(finalWord, texture.layers-firstLayer) : texture.layers-firstLayer;
            break;
        case 3: // D3
            if (texture.dimension != 3) return false;
            type=VK_IMAGE_VIEW_TYPE_3D;
            baseLayer=0;
            break;
        case 4: // Cube
            if (texture.dimension != 2 || texture.layers < 6 || (texture.layers % 6u)) return false;
            type=VK_IMAGE_VIEW_TYPE_CUBE;
            layerCount=6;
            break;
        case 5: // CubeArray; SRV count is in cubes, UAV has no explicit count
            if (texture.dimension != 2 || texture.layers < 6 || (texture.layers % 6u)) return false;
            type=VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            layerCount=(!unorderedAccess && finalWord) ? finalWord*6u : texture.layers-firstLayer;
            break;
        default:
            return false;
    }
    if (baseLayer >= texture.layers && texture.dimension != 3) return false;
    if (texture.dimension != 3 && (layerCount == 0 || layerCount > texture.layers-baseLayer)) return false;
    if ((type == VK_IMAGE_VIEW_TYPE_CUBE || type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) &&
        ((baseLayer % 6u) != 0 || (layerCount % 6u) != 0)) return false;

    // The current image allocation is not MUTABLE_FORMAT. Only the original
    // resource format (or zero, meaning inherit) is legal for this first view path.
    if (viewFormat != 0 && viewFormat != texture.sourceFormat) return false;
    VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    info.image=texture.image;
    info.viewType=type;
    info.format=texture.format;
    info.subresourceRange.aspectMask=isDepthVkFormat(texture.format)
        ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    if (texture.format == VK_FORMAT_D24_UNORM_S8_UINT ||
        texture.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    info.subresourceRange.baseMipLevel=baseMip;
    info.subresourceRange.levelCount=mipCount;
    info.subresourceRange.baseArrayLayer=baseLayer;
    info.subresourceRange.layerCount=texture.dimension == 3 ? 1u : layerCount;
    ViewResource view{};
    view.resourceId=resourceId;
    view.unorderedAccess=unorderedAccess;
    if (vkCreateImageView(device_, &info, nullptr, &view.view) != VK_SUCCESS) return false;
    views_.emplace(id, view);
    return true;
}


bool PGAndroidGpuResources::getBufferDescriptor(uint32_t id, VkDeviceSize offset,
                                                 VkDeviceSize range,
                                                 VkDescriptorBufferInfo &out) const {
    const auto it = buffers_.find(id);
    if (it == buffers_.end() || offset >= it->second.size) return false;
    const VkDeviceSize available = it->second.size - offset;
    const VkDeviceSize resolvedRange = range == VK_WHOLE_SIZE ? available : range;
    if (!resolvedRange || resolvedRange > available) return false;
    out = {};
    out.buffer = it->second.buffer;
    out.offset = offset;
    out.range = resolvedRange;
    return out.buffer != VK_NULL_HANDLE;
}

bool PGAndroidGpuResources::getImageDescriptor(uint32_t id,
                                                VkDescriptorImageInfo &out) const {
    const auto it = views_.find(id);
    if (it == views_.end() || !it->second.view) return false;
    const auto texture = textures_.find(it->second.resourceId);
    if (texture == textures_.end()) return false;
    out = {};
    out.imageView = it->second.view;
    if (texture->second.layout == VK_IMAGE_LAYOUT_UNDEFINED) return false;
    // Descriptor layout must describe the image's actual current layout. A later
    // command barrier may transition it before binding/use.
    out.imageLayout = texture->second.layout;
    return true;
}

bool PGAndroidGpuResources::destroyObject(uint32_t id) {
    const auto viewIt = views_.find(id);
    if (viewIt != views_.end()) {
        if (vkDeviceWaitIdle(device_) != VK_SUCCESS) return false;
        if (viewIt->second.view) vkDestroyImageView(device_, viewIt->second.view, nullptr);
        views_.erase(viewIt);
        return true;
    }
    const auto bufferIt = buffers_.find(id);
    const auto textureIt = textures_.find(id);
    if (bufferIt == buffers_.end() && textureIt == textures_.end()) return false;
    // Until deferred destruction is implemented, wait for all queued GPU uses
    // before releasing either object type.
    if (vkDeviceWaitIdle(device_) != VK_SUCCESS) return false;
    auto it = buffers_.find(id);
    if (it == buffers_.end()) {
        auto texture = textures_.find(id);
        // Vulkan image views must be destroyed before their parent image.
        for (auto view = views_.begin(); view != views_.end();) {
            if (view->second.resourceId == id) {
                if (view->second.view) vkDestroyImageView(device_, view->second.view, nullptr);
                view = views_.erase(view);
            } else {
                ++view;
            }
        }
        if (texture->second.view) vkDestroyImageView(device_, texture->second.view, nullptr);
        if (texture->second.image) vkDestroyImage(device_, texture->second.image, nullptr);
        if (texture->second.memory) vkFreeMemory(device_, texture->second.memory, nullptr);
        textures_.erase(texture);
        return true;
    }
    if (it->second.mapped) vkUnmapMemory(device_, it->second.memory);
    vkDestroyBuffer(device_, it->second.buffer, nullptr);
    vkFreeMemory(device_, it->second.memory, nullptr);
    buffers_.erase(it);
    return true;
}

void PGAndroidGpuResources::reset() {
    for (auto &entry : views_) if (entry.second.view) vkDestroyImageView(device_, entry.second.view, nullptr);
    views_.clear();
    for (auto &entry : buffers_) {
        auto &resource = entry.second;
        if (resource.mapped) vkUnmapMemory(device_, resource.memory);
        if (resource.buffer) vkDestroyBuffer(device_, resource.buffer, nullptr);
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
    }
    buffers_.clear();
    for (auto &entry : textures_) {
        auto &resource=entry.second;
        if (resource.view) vkDestroyImageView(device_, resource.view, nullptr);
        if (resource.image) vkDestroyImage(device_, resource.image, nullptr);
        if (resource.memory) vkFreeMemory(device_, resource.memory, nullptr);
    }
    textures_.clear();
}
