// File: Source/Filament/Internal/VulkanGraphicsInterface/Pipeline/VkPipelineForge.h
#pragma once

/*====================================================================================================================================
                                                     VKPIPELINEFORGE.H
====================================================================================================================================*/
// 🧩 Graphics and compute pipeline construction for Filament.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../Context/VkBootstrap.h"
#include <vector>
#include <string>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    SHADER MODULE
//------------------------------------------------------------------------------------------------------------------------

namespace VkPipelineForge
{
    // ① Load SPIR-V from file and create a shader module
    [[nodiscard]] VkShaderModule forgeShaderModule(VkDevice device, const std::string& spvPath) noexcept(false);

    // ② Construct a graphics pipeline for the GBuffer pass
    [[nodiscard]] VkPipeline forgeGBufferPipeline(
        VkDevice              device,
        VkPipelineLayout      layout,
        VkRenderPass          renderPass,
        uint32_t              subpass,
        VkShaderModule        vertModule,
        VkShaderModule        fragModule,
        uint32_t              colorAttachmentCount) noexcept(false);

    // ③ Construct a compute pipeline
    [[nodiscard]] VkPipeline forgeComputePipeline(
        VkDevice              device,
        VkPipelineLayout      layout,
        VkShaderModule        compModule) noexcept(false);

    // ④ Construct a graphics pipeline for the ImGui pass (fullscreen)
    [[nodiscard]] VkPipeline forgeFullscreenPipeline(
        VkDevice              device,
        VkPipelineLayout      layout,
        VkRenderPass          renderPass,
        uint32_t              subpass,
        VkShaderModule        vertModule,
        VkShaderModule        fragModule) noexcept(false);
}
