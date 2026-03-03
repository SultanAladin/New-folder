// File: Source/Filament/Internal/VulkanGraphicsInterface/Debug/VkValidation.h
#pragma once

/*====================================================================================================================================
                                                       VKVALIDATION.H
====================================================================================================================================*/
// 🧩 Debug validation layers and messenger callback for Filament.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include <vector>
#include <cstdio>

//------------------------------------------------------------------------------------------------------------------------
//                                                    VALIDATION LAYERS
//------------------------------------------------------------------------------------------------------------------------

namespace VkValidation
{
    // ⚙️ Requested validation layer names
    inline const std::vector<const char*>& queryRequestedLayers() noexcept
    {
        static const std::vector<const char*> layers = {
            "VK_LAYER_KHRONOS_validation"
        };
        return layers;
    }

    // ⚙️ Check whether all requested layers are available
    [[nodiscard]] bool confirmLayerSupport() noexcept;

    // ⚙️ Debug messenger callback — logs Vulkan validation messages
    VKAPI_ATTR VkBool32 VKAPI_CALL dispatchDebugMessage(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void*                                       userData);

    // ⚙️ Populate creation info for the debug messenger
    void prepareMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) noexcept;

    // ⚙️ Construct the debug messenger via extension function pointer
    VkResult constructDebugMessenger(VkInstance                                instance,
                                     const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
                                     const VkAllocationCallbacks*              allocator,
                                     VkDebugUtilsMessengerEXT*                 messenger) noexcept;

    // ⚙️ Retire the debug messenger via extension function pointer
    void retireDebugMessenger(VkInstance                   instance,
                              VkDebugUtilsMessengerEXT     messenger,
                              const VkAllocationCallbacks* allocator) noexcept;
}
