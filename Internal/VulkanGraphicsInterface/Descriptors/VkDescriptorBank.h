// File: Source/Filament/Internal/VulkanGraphicsInterface/Descriptors/VkDescriptorBank.h
#pragma once

/*====================================================================================================================================
                                                    VKDESCRIPTORBANK.H
====================================================================================================================================*/
// 🧩 Descriptor set layouts, pools, allocation, and write operations.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../Context/VkBootstrap.h"
#include "../Memory/VkMemoryReservoir.h"
#include <cstdint>
#include <vector>

//------------------------------------------------------------------------------------------------------------------------
//                                                    DESCRIPTOR LAYOUTS
//------------------------------------------------------------------------------------------------------------------------

// 📌 Set 0: Scene UBO (camera, lights, time)
// 📌 Set 1: Per-material UBO (64 bytes)
// 📌 Set 2: GBuffer images + sampler (for lighting/refraction passes)

struct FilamentDescriptorLayouts
{
    VkDescriptorSetLayout sceneLayout    = VK_NULL_HANDLE;  // [-] - Set 0
    VkDescriptorSetLayout materialLayout = VK_NULL_HANDLE;  // [-] - Set 1
    VkDescriptorSetLayout gbufferLayout  = VK_NULL_HANDLE;  // [-] - Set 2
    VkDescriptorSetLayout computeLayout  = VK_NULL_HANDLE;  // [-] - Compute pass (input + output images)
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace VkDescriptorBank
{
    // ① Construct all descriptor set layouts
    void constructLayouts(VkDevice device, FilamentDescriptorLayouts& layouts) noexcept(false);

    // ② Retire all descriptor set layouts
    void retireLayouts(VkDevice device, FilamentDescriptorLayouts& layouts) noexcept;

    // ③ Construct a descriptor pool
    [[nodiscard]] VkDescriptorPool constructPool(VkDevice device,
                                                  uint32_t maxSets,
                                                  const std::vector<VkDescriptorPoolSize>& sizes) noexcept(false);

    // ④ Allocate descriptor sets from a pool
    void allocateSets(VkDevice              device,
                      VkDescriptorPool      pool,
                      const VkDescriptorSetLayout* layouts,
                      uint32_t              count,
                      VkDescriptorSet*      outSets) noexcept(false);

    // ⑤ Write a UBO to a descriptor set at a binding
    void writeBufferDescriptor(VkDevice         device,
                               VkDescriptorSet  dstSet,
                               uint32_t         binding,
                               VkBuffer         buffer,
                               VkDeviceSize     offset,
                               VkDeviceSize     range) noexcept;

    // ⑥ Write a combined image sampler to a descriptor set
    void writeImageDescriptor(VkDevice         device,
                              VkDescriptorSet  dstSet,
                              uint32_t         binding,
                              VkImageView      imageView,
                              VkSampler        sampler,
                              VkImageLayout    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) noexcept;

    // ⑦ Write a storage image to a descriptor set
    void writeStorageImageDescriptor(VkDevice         device,
                                     VkDescriptorSet  dstSet,
                                     uint32_t         binding,
                                     VkImageView      imageView) noexcept;

    // ⑧ Construct pipeline layouts from descriptor set layouts
    [[nodiscard]] VkPipelineLayout constructGBufferPipelineLayout(
        VkDevice device, const FilamentDescriptorLayouts& layouts) noexcept(false);

    [[nodiscard]] VkPipelineLayout constructComputePipelineLayout(
        VkDevice device, const FilamentDescriptorLayouts& layouts) noexcept(false);
}
