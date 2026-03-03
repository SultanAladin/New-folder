// File: Source/Filament/Internal/VulkanGraphicsInterface/Memory/VkMemoryReservoir.cpp

/*====================================================================================================================================
                                                  VKMEMORYRESERVOIR.CPP
====================================================================================================================================*/
// 🧩 GPU buffer/image allocation and staging upload implementation.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VkMemoryReservoir.h"
#include "../../Auxiliary/FilamentTypes.h"

#include <cstring>

namespace VkMemory
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  BUFFER ALLOCATION
//------------------------------------------------------------------------------------------------------------------------

void allocateBuffer(const VkBootstrapContext& ctx,
                    VkDeviceSize              size,
                    VkBufferUsageFlags        usage,
                    VkMemoryPropertyFlags     memProps,
                    GpuBuffer&                buffer) noexcept(false)
{
    VkBufferCreateInfo bufInfo = {};
    bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size        = size;
    bufInfo.usage       = usage;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    FILAMENT_VK_VERIFY(vkCreateBuffer(ctx.device, &bufInfo, nullptr, &buffer.buffer));

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx.device, buffer.buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReqs.size;
    allocInfo.memoryTypeIndex = VkBootstrap::locateMemoryType(ctx, memReqs.memoryTypeBits, memProps);

    FILAMENT_VK_VERIFY(vkAllocateMemory(ctx.device, &allocInfo, nullptr, &buffer.memory));
    FILAMENT_VK_VERIFY(vkBindBufferMemory(ctx.device, buffer.buffer, buffer.memory, 0));
    buffer.size = size;

    // ⚙️ Persistently map host-visible buffers
    if (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(ctx.device, buffer.memory, 0, size, 0, &buffer.mapped);
    }
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  BUFFER RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void retireBuffer(VkDevice device, GpuBuffer& buffer) noexcept
{
    if (buffer.mapped)
    {
        vkUnmapMemory(device, buffer.memory);
        buffer.mapped = nullptr;
    }
    if (buffer.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, buffer.buffer, nullptr);
    if (buffer.memory != VK_NULL_HANDLE) vkFreeMemory(device, buffer.memory, nullptr);
    buffer = {};
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  STAGED UPLOAD
//------------------------------------------------------------------------------------------------------------------------

void uploadToDeviceBuffer(const VkBootstrapContext& ctx,
                          VkCommandPool             pool,
                          const void*               data,
                          VkDeviceSize              size,
                          GpuBuffer&                deviceBuffer) noexcept(false)
{
    // ① Allocate a staging buffer (host-visible)
    GpuBuffer staging;
    allocateBuffer(ctx, size,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                   staging);

    // ② Copy data to staging
    std::memcpy(staging.mapped, data, static_cast<size_t>(size));

    // ③ Record and submit the copy command
    VkCommandBuffer cmd = VkCommandDispatch::beginTransient(ctx.device, pool);

    VkBufferCopy region = {};
    region.size = size;
    vkCmdCopyBuffer(cmd, staging.buffer, deviceBuffer.buffer, 1, &region);

    VkCommandDispatch::submitTransient(ctx.device, ctx.graphicsQueue, pool, cmd);

    // ④ Retire staging buffer
    retireBuffer(ctx.device, staging);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  IMAGE ALLOCATION
//------------------------------------------------------------------------------------------------------------------------

void allocateImage2D(const VkBootstrapContext& ctx,
                     uint32_t                  width,
                     uint32_t                  height,
                     VkFormat                  format,
                     VkImageUsageFlags         usage,
                     VkImageAspectFlags        aspect,
                     GpuImage&                 image) noexcept(false)
{
    // ① Create image
    VkImageCreateInfo imgInfo = {};
    imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType     = VK_IMAGE_TYPE_2D;
    imgInfo.format        = format;
    imgInfo.extent        = { width, height, 1 };
    imgInfo.mipLevels     = 1;
    imgInfo.arrayLayers   = 1;
    imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage         = usage;
    imgInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    FILAMENT_VK_VERIFY(vkCreateImage(ctx.device, &imgInfo, nullptr, &image.image));

    // ② Allocate device-local memory
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(ctx.device, image.image, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReqs.size;
    allocInfo.memoryTypeIndex = VkBootstrap::locateMemoryType(ctx, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    FILAMENT_VK_VERIFY(vkAllocateMemory(ctx.device, &allocInfo, nullptr, &image.memory));
    FILAMENT_VK_VERIFY(vkBindImageMemory(ctx.device, image.image, image.memory, 0));

    // ③ Create image view
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = image.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = format;
    viewInfo.subresourceRange.aspectMask     = aspect;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    FILAMENT_VK_VERIFY(vkCreateImageView(ctx.device, &viewInfo, nullptr, &image.view));

    image.format = format;
    image.width  = width;
    image.height = height;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  IMAGE RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void retireImage(VkDevice device, GpuImage& image) noexcept
{
    if (image.view   != VK_NULL_HANDLE) vkDestroyImageView(device, image.view, nullptr);
    if (image.image  != VK_NULL_HANDLE) vkDestroyImage(device, image.image, nullptr);
    if (image.memory != VK_NULL_HANDLE) vkFreeMemory(device, image.memory, nullptr);
    image = {};
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  SAMPLER CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

VkSampler constructLinearSampler(VkDevice device) noexcept(false)
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter     = VK_FILTER_LINEAR;
    samplerInfo.minFilter     = VK_FILTER_LINEAR;
    samplerInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod        = 1.0f;

    VkSampler sampler;
    FILAMENT_VK_VERIFY(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));
    return sampler;
}

} // namespace VkMemory
