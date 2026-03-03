// File: Source/Filament/Internal/RenderSubsystem/FilamentGBuffer.h
#pragma once

/*====================================================================================================================================
                                                    FILAMENTGBUFFER.H
====================================================================================================================================*/
// 🧩 GBuffer pass — 3 render targets + depth for deferred PBR shading.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../VulkanGraphicsInterface/Context/VkBootstrap.h"
#include "../VulkanGraphicsInterface/Memory/VkMemoryReservoir.h"
#include <array>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    GBUFFER STATE
//------------------------------------------------------------------------------------------------------------------------

// 📌 RT0: R8G8B8A8_UNORM  — Albedo.RGB + Metallic.A        (32 bpp)
// 📌 RT1: R16G16_SFLOAT   — Normal.XY (octahedral encoded) (32 bpp)
// 📌 RT2: R8G8_UNORM      — Roughness.R + Specular.G       (16 bpp)
// 📌 Depth: D32_SFLOAT    — Hardware depth                  (32 bpp)

struct FilamentGBufferState
{
    std::array<GpuImage, 3> colorTargets;        // [-] - RT0, RT1, RT2
    GpuImage                depthTarget;          // [-] - Depth buffer
    VkRenderPass            renderPass = VK_NULL_HANDLE;  // [-] - GBuffer render pass
    VkFramebuffer           framebuffer = VK_NULL_HANDLE; // [-] - GBuffer framebuffer
    uint32_t                width  = 0;          // [px] - Render target width
    uint32_t                height = 0;          // [px] - Render target height
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace FilamentGBuffer
{
    // ① Construct GBuffer render targets, render pass, and framebuffer
    void constructGBuffer(const VkBootstrapContext& ctx,
                          FilamentGBufferState&     state,
                          uint32_t                  width,
                          uint32_t                  height) noexcept(false);

    // ② Retire all GBuffer resources
    void retireGBuffer(VkDevice device, FilamentGBufferState& state) noexcept;

    // ③ Recreate on resize
    void recreateGBuffer(const VkBootstrapContext& ctx,
                         FilamentGBufferState&     state,
                         uint32_t                  width,
                         uint32_t                  height) noexcept(false);
}
