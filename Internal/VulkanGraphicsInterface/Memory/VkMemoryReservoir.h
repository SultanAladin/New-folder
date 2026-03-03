// File: Source/Filament/Internal/VulkanGraphicsInterface/Memory/VkMemoryReservoir.h
#pragma once

/*====================================================================================================================================
                                                   VKMEMORYRESERVOIR.H
====================================================================================================================================*/
// 🧩 GPU memory allocation, buffer/image construction, staging upload.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../Context/VkBootstrap.h"
#include "../Commands/VkCommandDispatch.h"
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    BUFFER RESOURCE
//------------------------------------------------------------------------------------------------------------------------

struct GpuBuffer
{
    VkBuffer       buffer = VK_NULL_HANDLE;      // [-] - Buffer handle
    VkDeviceMemory memory = VK_NULL_HANDLE;      // [-] - Backing memory
    VkDeviceSize   size   = 0;                   // [B] - Byte size
    void*          mapped = nullptr;             // [-] - Persistently mapped pointer (if host-visible)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    IMAGE RESOURCE
//------------------------------------------------------------------------------------------------------------------------

struct GpuImage
{
    VkImage        image      = VK_NULL_HANDLE;  // [-] - Image handle
    VkDeviceMemory memory     = VK_NULL_HANDLE;  // [-] - Backing memory
    VkImageView    view       = VK_NULL_HANDLE;  // [-] - Default view
    VkFormat       format     = VK_FORMAT_UNDEFINED; // [-] - Pixel format
    uint32_t       width      = 0;               // [px] - Width
    uint32_t       height     = 0;               // [px] - Height
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace VkMemory
{
    // ① Allocate a GPU buffer (device-local or host-visible)
    void allocateBuffer(const VkBootstrapContext& ctx,
                        VkDeviceSize              size,
                        VkBufferUsageFlags        usage,
                        VkMemoryPropertyFlags     memProps,
                        GpuBuffer&                buffer) noexcept(false);

    // ② Retire a GPU buffer
    void retireBuffer(VkDevice device, GpuBuffer& buffer) noexcept;

    // ③ Upload data to a device-local buffer via staging
    void uploadToDeviceBuffer(const VkBootstrapContext& ctx,
                              VkCommandPool             pool,
                              const void*               data,
                              VkDeviceSize              size,
                              GpuBuffer&                deviceBuffer) noexcept(false);

    // ④ Allocate a 2D image with a view
    void allocateImage2D(const VkBootstrapContext& ctx,
                         uint32_t                  width,
                         uint32_t                  height,
                         VkFormat                  format,
                         VkImageUsageFlags         usage,
                         VkImageAspectFlags        aspect,
                         GpuImage&                 image) noexcept(false);

    // ⑤ Retire a GPU image + view
    void retireImage(VkDevice device, GpuImage& image) noexcept;

    // ⑥ Create a sampler with linear filtering and clamp-to-edge
    VkSampler constructLinearSampler(VkDevice device) noexcept(false);
}
