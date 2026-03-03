// File: Source/Filament/Internal/VulkanGraphicsInterface/Context/VkBootstrap.h
#pragma once

/*====================================================================================================================================
                                                       VKBOOTSTRAP.H
====================================================================================================================================*/
// 🧩 Vulkan instance, physical device, logical device, and queue construction.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <vector>
#include <cstdint>
#include <optional>

//------------------------------------------------------------------------------------------------------------------------
//                                                    FORWARD DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

struct VkBootstrapContext
{
    VkInstance                    instance        = VK_NULL_HANDLE;  // [-] - Vulkan instance
    VkDebugUtilsMessengerEXT     debugMessenger   = VK_NULL_HANDLE;  // [-] - Debug messenger (debug only)
    VkSurfaceKHR                 surface          = VK_NULL_HANDLE;  // [-] - Window surface
    VkPhysicalDevice             physicalDevice   = VK_NULL_HANDLE;  // [-] - Selected GPU
    VkDevice                     device           = VK_NULL_HANDLE;  // [-] - Logical device
    VkQueue                      graphicsQueue    = VK_NULL_HANDLE;  // [-] - Graphics + present queue
    uint32_t                     graphicsFamily   = 0;               // [idx] - Queue family index
    VkPhysicalDeviceProperties   deviceProperties = {};              // [-] - Physical device caps
    VkPhysicalDeviceMemoryProperties memProperties = {};             // [-] - Memory heaps
    bool                         validationEnabled = false;          // [-] - Whether validation is active
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace VkBootstrap
{
    // ① Construct Vulkan instance with extensions and optional validation
    void constructInstance(VkBootstrapContext& ctx, bool enableValidation) noexcept(false);

    // ② Construct Win32 surface from HWND + HINSTANCE
    void constructSurface(VkBootstrapContext& ctx, HWND windowHandle, HINSTANCE appInstance) noexcept(false);

    // ③ Select the best physical device (discrete GPU preferred)
    void selectPhysicalDevice(VkBootstrapContext& ctx) noexcept(false);

    // ④ Construct logical device and retrieve queues
    void constructDevice(VkBootstrapContext& ctx) noexcept(false);

    // ⑤ Retire all Vulkan resources in reverse order
    void retireContext(VkBootstrapContext& ctx) noexcept;

    // ⚙️ Locate a memory type matching the filter and property flags
    [[nodiscard]] uint32_t locateMemoryType(const VkBootstrapContext& ctx,
                                            uint32_t                  typeFilter,
                                            VkMemoryPropertyFlags     properties) noexcept(false);
}
