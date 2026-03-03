// File: Source/Filament/Internal/VulkanGraphicsInterface/Context/VkBootstrap.cpp

/*====================================================================================================================================
                                                      VKBOOTSTRAP.CPP
====================================================================================================================================*/
// 🧩 Vulkan instance, device, surface creation and teardown.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VkBootstrap.h"
#include "../Debug/VkValidation.h"
#include "../../Auxiliary/FilamentLog.h"
#include "../../Auxiliary/FilamentTypes.h"

#include <vector>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace VkBootstrap
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  INSTANCE CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructInstance(VkBootstrapContext& ctx, bool enableValidation) noexcept(false)
{
    // ① Check validation layer support
    if (enableValidation && VkValidation::confirmLayerSupport())
    {
        ctx.validationEnabled = true;
        FilamentLog::info("Validation layers enabled.");
    }
    else if (enableValidation)
    {
        FilamentLog::warn("Validation layers requested but not available — continuing without.");
    }

    // ② Application and instance info
    VkApplicationInfo appInfo = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Filament";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName        = "Filament";
    appInfo.engineVersion      = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion         = VK_MAKE_API_VERSION(0, 1, 4, 335);

    // ③ Required instance extensions
    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    if (ctx.validationEnabled)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (ctx.validationEnabled)
    {
        const auto& layers              = VkValidation::queryRequestedLayers();
        createInfo.enabledLayerCount    = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames  = layers.data();

        VkValidation::prepareMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    FILAMENT_VK_VERIFY(vkCreateInstance(&createInfo, nullptr, &ctx.instance));
    FilamentLog::info("Vulkan instance constructed (API 1.4.335).");

    // ④ Create debug messenger
    if (ctx.validationEnabled)
    {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
        VkValidation::prepareMessengerCreateInfo(messengerInfo);
        VkValidation::constructDebugMessenger(ctx.instance, &messengerInfo, nullptr, &ctx.debugMessenger);
    }
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  SURFACE CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructSurface(VkBootstrapContext& ctx, HWND windowHandle, HINSTANCE appInstance) noexcept(false)
{
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd      = windowHandle;
    surfaceInfo.hinstance = appInstance;

    FILAMENT_VK_VERIFY(vkCreateWin32SurfaceKHR(ctx.instance, &surfaceInfo, nullptr, &ctx.surface));
    FilamentLog::info("Win32 surface constructed.");
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  PHYSICAL DEVICE SELECTION
//------------------------------------------------------------------------------------------------------------------------

void selectPhysicalDevice(VkBootstrapContext& ctx) noexcept(false)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
    if (deviceCount == 0) throw std::runtime_error("No Vulkan-capable GPU found.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data());

    // ⚙️ Prefer discrete GPU, fallback to integrated
    VkPhysicalDevice bestDevice   = VK_NULL_HANDLE;
    int              bestScore    = -1;

    for (auto& candidate : devices)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(candidate, &props);

        // ⚙️ Verify queue family for graphics + present
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());

        bool hasGraphicsQueue = false;
        for (uint32_t i = 0; i < familyCount; ++i)
        {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, ctx.surface, &presentSupport);
                if (presentSupport) { hasGraphicsQueue = true; break; }
            }
        }
        if (!hasGraphicsQueue) continue;

        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)   score += 1000;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 100;

        if (score > bestScore)
        {
            bestScore  = score;
            bestDevice = candidate;
        }
    }

    if (bestDevice == VK_NULL_HANDLE) throw std::runtime_error("No suitable GPU found with graphics + present support.");

    ctx.physicalDevice = bestDevice;
    vkGetPhysicalDeviceProperties(ctx.physicalDevice, &ctx.deviceProperties);
    vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &ctx.memProperties);

    FilamentLog::info("Selected GPU: %s", ctx.deviceProperties.deviceName);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  LOGICAL DEVICE CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructDevice(VkBootstrapContext& ctx) noexcept(false)
{
    // ① Find graphics + present queue family
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physicalDevice, &familyCount, families.data());

    for (uint32_t i = 0; i < familyCount; ++i)
    {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(ctx.physicalDevice, i, ctx.surface, &presentSupport);
            if (presentSupport)
            {
                ctx.graphicsFamily = i;
                break;
            }
        }
    }

    // ② Queue creation
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = ctx.graphicsFamily;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    // ③ Device extensions
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // ④ Device features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    // ⑤ Synchronization2 feature (Vulkan 1.3+)
    VkPhysicalDeviceSynchronization2Features sync2Features = {};
    sync2Features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Features.synchronization2 = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures dynRenderFeatures = {};
    dynRenderFeatures.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynRenderFeatures.dynamicRendering = VK_TRUE;
    dynRenderFeatures.pNext            = &sync2Features;

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext                   = &dynRenderFeatures;
    deviceInfo.queueCreateInfoCount    = 1;
    deviceInfo.pQueueCreateInfos       = &queueInfo;
    deviceInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceInfo.pEnabledFeatures        = &deviceFeatures;

    FILAMENT_VK_VERIFY(vkCreateDevice(ctx.physicalDevice, &deviceInfo, nullptr, &ctx.device));
    vkGetDeviceQueue(ctx.device, ctx.graphicsFamily, 0, &ctx.graphicsQueue);

    FilamentLog::info("Logical device constructed (queue family %u).", ctx.graphicsFamily);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  CONTEXT TEARDOWN
//------------------------------------------------------------------------------------------------------------------------

void retireContext(VkBootstrapContext& ctx) noexcept
{
    // 🔴 CRITICAL: Must destroy in reverse creation order
    if (ctx.device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(ctx.device);
        vkDestroyDevice(ctx.device, nullptr);
        ctx.device = VK_NULL_HANDLE;
    }
    if (ctx.surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
        ctx.surface = VK_NULL_HANDLE;
    }
    if (ctx.debugMessenger != VK_NULL_HANDLE)
    {
        VkValidation::retireDebugMessenger(ctx.instance, ctx.debugMessenger, nullptr);
        ctx.debugMessenger = VK_NULL_HANDLE;
    }
    if (ctx.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(ctx.instance, nullptr);
        ctx.instance = VK_NULL_HANDLE;
    }

    FilamentLog::info("Vulkan context retired.");
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  MEMORY TYPE LOOKUP
//------------------------------------------------------------------------------------------------------------------------

uint32_t locateMemoryType(const VkBootstrapContext& ctx,
                          uint32_t                  typeFilter,
                          VkMemoryPropertyFlags     properties) noexcept(false)
{
    for (uint32_t i = 0; i < ctx.memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1u << i)) &&
            (ctx.memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("Failed to locate suitable memory type.");
}

} // namespace VkBootstrap
