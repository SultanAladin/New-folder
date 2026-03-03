// File: Source/Filament/Internal/VulkanGraphicsInterface/Commands/VkCommandDispatch.cpp

/*====================================================================================================================================
                                                   VKCOMMANDDISPATCH.CPP
====================================================================================================================================*/
// 🧩 Command pool and command buffer management implementation.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VkCommandDispatch.h"
#include "../../Auxiliary/FilamentTypes.h"

namespace VkCommandDispatch
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  POOL CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructPool(VkDevice device, uint32_t queueFamily,
                   VkCommandPool& pool, VkCommandPoolCreateFlags flags) noexcept(false)
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamily;
    poolInfo.flags            = flags;

    FILAMENT_VK_VERIFY(vkCreateCommandPool(device, &poolInfo, nullptr, &pool));
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  BUFFER ALLOCATION
//------------------------------------------------------------------------------------------------------------------------

void allocateBuffers(VkDevice device, VkCommandPool pool,
                     uint32_t count, std::vector<VkCommandBuffer>& buffers) noexcept(false)
{
    buffers.resize(count);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = pool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    FILAMENT_VK_VERIFY(vkAllocateCommandBuffers(device, &allocInfo, buffers.data()));
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  TRANSIENT COMMANDS
//------------------------------------------------------------------------------------------------------------------------

VkCommandBuffer beginTransient(VkDevice device, VkCommandPool pool) noexcept(false)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = pool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    FILAMENT_VK_VERIFY(vkAllocateCommandBuffers(device, &allocInfo, &cmd));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    FILAMENT_VK_VERIFY(vkBeginCommandBuffer(cmd, &beginInfo));
    return cmd;
}

void submitTransient(VkDevice device, VkQueue queue, VkCommandPool pool,
                     VkCommandBuffer cmd) noexcept(false)
{
    FILAMENT_VK_VERIFY(vkEndCommandBuffer(cmd));

    VkSubmitInfo submitInfo = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    FILAMENT_VK_VERIFY(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, &cmd);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  POOL RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void retirePool(VkDevice device, VkCommandPool& pool) noexcept
{
    if (pool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, pool, nullptr);
        pool = VK_NULL_HANDLE;
    }
}

} // namespace VkCommandDispatch
