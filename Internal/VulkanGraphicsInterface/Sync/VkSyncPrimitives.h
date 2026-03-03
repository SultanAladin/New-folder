// File: Source/Filament/Internal/VulkanGraphicsInterface/Sync/VkSyncPrimitives.h
#pragma once

/*====================================================================================================================================
                                                   VKSYNCPRIMITIVES.H
====================================================================================================================================*/
// 🧩 Semaphores, fences, and Synchronization2 barrier helpers.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../../Auxiliary/FilamentTypes.h"
#include <array>

//------------------------------------------------------------------------------------------------------------------------
//                                                    FRAME SYNC
//------------------------------------------------------------------------------------------------------------------------

struct FrameSyncPrimitives
{
    std::array<VkSemaphore, FILAMENT_MAX_FRAMES_IN_FLIGHT> imageAvailable = {};  // [-] - Signaled when swapchain image acquired
    std::array<VkSemaphore, FILAMENT_MAX_FRAMES_IN_FLIGHT> renderFinished = {};  // [-] - Signaled when rendering complete
    std::array<VkFence,     FILAMENT_MAX_FRAMES_IN_FLIGHT> inFlightFence  = {};  // [-] - CPU-GPU sync per frame
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace VkSync
{
    // ① Construct all per-frame sync primitives
    void constructFrameSync(VkDevice device, FrameSyncPrimitives& sync) noexcept(false);

    // ② Retire all per-frame sync primitives
    void retireFrameSync(VkDevice device, FrameSyncPrimitives& sync) noexcept;

    // ③ Image layout transition using Synchronization2 pipeline barriers
    void transitionImageLayout(VkCommandBuffer   cmd,
                               VkImage           image,
                               VkImageLayout     oldLayout,
                               VkImageLayout     newLayout,
                               VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT) noexcept;

    // ④ Pipeline barrier for compute → fragment read dependency
    void barrierComputeToFragment(VkCommandBuffer cmd,
                                  VkImage         image) noexcept;
}
