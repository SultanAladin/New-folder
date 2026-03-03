// File: Source/Filament/Internal/VulkanGraphicsInterface/Resources/VkSwapchain.cpp

/*====================================================================================================================================
                                                      VKSWAPCHAIN.CPP
====================================================================================================================================*/
// 🧩 Swapchain construction, image view creation, and resize handling.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VkSwapchain.h"
#include "../../Auxiliary/FilamentLog.h"
#include "../../Auxiliary/FilamentTypes.h"

#include <algorithm>
#include <stdexcept>

namespace VkSwapchainOps
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  SWAPCHAIN CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructSwapchain(const VkBootstrapContext& ctx,
                        VkSwapchainState&         state,
                        uint32_t                  width,
                        uint32_t                  height,
                        VkSwapchainKHR            oldSwapchain) noexcept(false)
{
    // ① Query surface capabilities
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &caps);

    // ② Select surface format (prefer B8G8R8A8_SRGB)
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, formats.data());

    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (const auto& fmt : formats)
    {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosenFormat = fmt;
            break;
        }
    }
    state.imageFormat = chosenFormat.format;

    // ③ Select present mode (prefer mailbox → fifo fallback)
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice, ctx.surface, &modeCount, nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice, ctx.surface, &modeCount, modes.data());

    VkPresentModeKHR chosenMode = VK_PRESENT_MODE_FIFO_KHR;
    // 💡 Mailbox gives triple-buffering on most drivers; FIFO is the guaranteed fallback
    for (auto m : modes)
    {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) { chosenMode = m; break; }
    }

    // ④ Resolve extent
    if (caps.currentExtent.width != UINT32_MAX)
    {
        state.extent = caps.currentExtent;
    }
    else
    {
        state.extent.width  = std::clamp(width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
        state.extent.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    // ⑤ Image count (prefer min+1, capped by max)
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    // ⑥ Create swapchain
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = ctx.surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = chosenFormat.format;
    createInfo.imageColorSpace  = chosenFormat.colorSpace;
    createInfo.imageExtent      = state.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform     = caps.currentTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = chosenMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = oldSwapchain;

    FILAMENT_VK_VERIFY(vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &state.swapchain));

    // ⑦ Retrieve swapchain images
    vkGetSwapchainImagesKHR(ctx.device, state.swapchain, &state.imageCount, nullptr);
    state.images.resize(state.imageCount);
    vkGetSwapchainImagesKHR(ctx.device, state.swapchain, &state.imageCount, state.images.data());

    // ⑧ Create image views
    state.imageViews.resize(state.imageCount);
    for (uint32_t i = 0; i < state.imageCount; ++i)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = state.images[i];
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = state.imageFormat;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        FILAMENT_VK_VERIFY(vkCreateImageView(ctx.device, &viewInfo, nullptr, &state.imageViews[i]));
    }

    FilamentLog::info("Swapchain constructed: %ux%u, %u images.", state.extent.width, state.extent.height, state.imageCount);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  SWAPCHAIN RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void retireSwapchain(VkDevice device, VkSwapchainState& state) noexcept
{
    for (auto view : state.imageViews)
    {
        if (view != VK_NULL_HANDLE) vkDestroyImageView(device, view, nullptr);
    }
    state.imageViews.clear();
    state.images.clear();

    if (state.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, state.swapchain, nullptr);
        state.swapchain = VK_NULL_HANDLE;
    }
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  SWAPCHAIN RECREATION
//------------------------------------------------------------------------------------------------------------------------

void recreateSwapchain(const VkBootstrapContext& ctx,
                       VkSwapchainState&         state,
                       uint32_t                  width,
                       uint32_t                  height) noexcept(false)
{
    vkDeviceWaitIdle(ctx.device);

    VkSwapchainKHR oldSwapchain = state.swapchain;

    // ⚙️ Retire views only (old swapchain passed to construction for recycling)
    for (auto view : state.imageViews)
    {
        if (view != VK_NULL_HANDLE) vkDestroyImageView(ctx.device, view, nullptr);
    }
    state.imageViews.clear();
    state.images.clear();

    constructSwapchain(ctx, state, width, height, oldSwapchain);

    // ⚙️ Destroy old swapchain after new one is created
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ctx.device, oldSwapchain, nullptr);
    }

    FilamentLog::info("Swapchain recreated: %ux%u.", width, height);
}

} // namespace VkSwapchainOps
