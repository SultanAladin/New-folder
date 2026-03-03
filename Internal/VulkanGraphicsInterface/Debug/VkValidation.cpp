// File: Source/Filament/Internal/VulkanGraphicsInterface/Debug/VkValidation.cpp

/*====================================================================================================================================
                                                      VKVALIDATION.CPP
====================================================================================================================================*/
// 🧩 Debug validation layer and messenger implementation.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VkValidation.h"
#include <cstring>
#include <cstdio>

namespace VkValidation
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  LAYER SUPPORT CHECK
//------------------------------------------------------------------------------------------------------------------------

bool confirmLayerSupport() noexcept
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> available(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available.data());

    for (const char* requested : queryRequestedLayers())
    {
        bool found = false;
        for (const auto& layer : available)
        {
            if (std::strcmp(requested, layer.layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::fprintf(stderr, "[WARN]    Validation layer not available: %s\n", requested);
            return false;
        }
    }
    return true;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  DEBUG MESSENGER
//------------------------------------------------------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL dispatchDebugMessage(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*                                       userData)
{
    (void)type;
    (void)userData;

    const char* prefix = "[VK_DBG]  ";
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        prefix = "[VK_ERR]  ";
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        prefix = "[VK_WARN] ";

    std::fprintf(stderr, "%s%s\n", prefix, callbackData->pMessage);
    return VK_FALSE;
}

void prepareMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info) noexcept
{
    info = {};
    info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = dispatchDebugMessage;
    info.pUserData       = nullptr;
}

VkResult constructDebugMessenger(VkInstance                                instance,
                                 const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
                                 const VkAllocationCallbacks*              allocator,
                                 VkDebugUtilsMessengerEXT*                 messenger) noexcept
{
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func) return func(instance, createInfo, allocator, messenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void retireDebugMessenger(VkInstance                   instance,
                          VkDebugUtilsMessengerEXT     messenger,
                          const VkAllocationCallbacks* allocator) noexcept
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func) func(instance, messenger, allocator);
}

} // namespace VkValidation
