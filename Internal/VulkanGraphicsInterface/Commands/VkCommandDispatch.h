// File: Source/Filament/Internal/VulkanGraphicsInterface/Commands/VkCommandDispatch.h
#pragma once

/*====================================================================================================================================
                                                    VKCOMMANDDISPATCH.H
====================================================================================================================================*/
// 🧩 Command pool and command buffer allocation, recording helpers.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../Context/VkBootstrap.h"
#include <vector>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    COMMAND STATE
//------------------------------------------------------------------------------------------------------------------------

struct VkCommandState
{
    VkCommandPool                    pool     = VK_NULL_HANDLE;  // [-] - Command pool
    std::vector<VkCommandBuffer>     buffers;                    // [-] - Allocated command buffers
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace VkCommandDispatch
{
    // ① Construct a command pool for the given queue family
    void constructPool(VkDevice device, uint32_t queueFamily,
                       VkCommandPool& pool, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) noexcept(false);

    // ② Allocate command buffers from a pool
    void allocateBuffers(VkDevice device, VkCommandPool pool,
                         uint32_t count, std::vector<VkCommandBuffer>& buffers) noexcept(false);

    // ③ Begin a single-use command buffer (for staging transfers, etc.)
    [[nodiscard]] VkCommandBuffer beginTransient(VkDevice device, VkCommandPool pool) noexcept(false);

    // ④ Submit and wait on a single-use command buffer
    void submitTransient(VkDevice device, VkQueue queue, VkCommandPool pool,
                         VkCommandBuffer cmd) noexcept(false);

    // ⑤ Retire command pool and all owned buffers
    void retirePool(VkDevice device, VkCommandPool& pool) noexcept;
}
