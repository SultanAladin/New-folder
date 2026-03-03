// File: Source/Filament/Internal/VulkanGraphicsInterface/Resources/VkSwapchain.h
#pragma once

/*====================================================================================================================================
                                                       VKSWAPCHAIN.H
====================================================================================================================================*/
// 🧩 Swapchain construction, image acquisition, recreation on resize.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../Context/VkBootstrap.h"
#include <vector>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    SWAPCHAIN STATE
//------------------------------------------------------------------------------------------------------------------------

struct VkSwapchainState
{
    VkSwapchainKHR             swapchain   = VK_NULL_HANDLE; // [-] - Swapchain handle
    VkFormat                   imageFormat = VK_FORMAT_B8G8R8A8_SRGB; // [-] - Surface format
    VkExtent2D                 extent      = {};             // [px] - Current swapchain size
    std::vector<VkImage>       images;                       // [-] - Swapchain images
    std::vector<VkImageView>   imageViews;                   // [-] - Views into swapchain images
    uint32_t                   imageCount  = 0;              // [-] - Number of swapchain images
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace VkSwapchainOps
{
    // ① Construct swapchain with preferred format/mode
    void constructSwapchain(const VkBootstrapContext& ctx,
                            VkSwapchainState&         state,
                            uint32_t                  width,
                            uint32_t                  height,
                            VkSwapchainKHR            oldSwapchain = VK_NULL_HANDLE) noexcept(false);

    // ② Retire swapchain and its image views
    void retireSwapchain(VkDevice device, VkSwapchainState& state) noexcept;

    // ③ Recreate swapchain on resize (retires old, constructs new)
    void recreateSwapchain(const VkBootstrapContext& ctx,
                           VkSwapchainState&         state,
                           uint32_t                  width,
                           uint32_t                  height) noexcept(false);
}
