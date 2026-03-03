// File: Source/Filament/Internal/RenderSubsystem/FilamentPipeline.h
#pragma once

/*====================================================================================================================================
                                                    FILAMENTPIPELINE.H
====================================================================================================================================*/
// 🧩 Pipeline orchestrator — frame loop, GBuffer→Lighting→Refraction→Tonemap→Present.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../VulkanGraphicsInterface/Context/VkBootstrap.h"
#include "../VulkanGraphicsInterface/Resources/VkSwapchain.h"
#include "../VulkanGraphicsInterface/Commands/VkCommandDispatch.h"
#include "../VulkanGraphicsInterface/Sync/VkSyncPrimitives.h"
#include "../VulkanGraphicsInterface/Memory/VkMemoryReservoir.h"
#include "../VulkanGraphicsInterface/Pipeline/VkPipelineForge.h"
#include "../VulkanGraphicsInterface/Descriptors/VkDescriptorBank.h"
#include "FilamentGBuffer.h"
#include "../SceneContext/FilamentScene.h"
#include "../SceneContext/FilamentCamera.h"
#include "../Auxiliary/FilamentTypes.h"

#include <vector>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    PIPELINE STATE
//------------------------------------------------------------------------------------------------------------------------

struct FilamentPipelineState
{
    // ⚙️ GBuffer pass
    FilamentGBufferState        gbuffer;
    VkPipeline                  gbufferPipeline    = VK_NULL_HANDLE;
    VkPipelineLayout            gbufferPipeLayout  = VK_NULL_HANDLE;

    // ⚙️ Compute passes
    VkPipeline                  lightingPipeline   = VK_NULL_HANDLE;
    VkPipeline                  refractionPipeline = VK_NULL_HANDLE;
    VkPipeline                  tonemapPipeline    = VK_NULL_HANDLE;
    VkPipelineLayout            computePipeLayout  = VK_NULL_HANDLE;

    // ⚙️ Offscreen render results
    GpuImage                    litColorImage;     // [-] - Output of lighting pass
    GpuImage                    refractionImage;   // [-] - Output of refraction pass
    GpuImage                    tonemappedImage;   // [-] - Final offscreen output (viewport)

    // ⚙️ Descriptors
    FilamentDescriptorLayouts   descLayouts;
    VkDescriptorPool            descriptorPool     = VK_NULL_HANDLE;
    VkDescriptorSet             sceneDescSet       = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> materialDescSets;
    VkDescriptorSet             lightingDescSet    = VK_NULL_HANDLE;
    VkDescriptorSet             refractionDescSet  = VK_NULL_HANDLE;
    VkDescriptorSet             tonemapDescSet     = VK_NULL_HANDLE;

    // ⚙️ Scene UBO
    GpuBuffer                   sceneUboBuffer;

    // ⚙️ Material UBOs
    std::vector<GpuBuffer>      materialUboBuffers;

    // ⚙️ Mesh GPU buffers
    std::vector<GpuBuffer>      vertexBuffers;
    std::vector<GpuBuffer>      indexBuffers;
    std::vector<uint32_t>       indexCounts;

    // ⚙️ Sampler
    VkSampler                   linearSampler      = VK_NULL_HANDLE;

    // ⚙️ Shader modules (retained for cleanup)
    VkShaderModule              gbufferVertModule   = VK_NULL_HANDLE;
    VkShaderModule              gbufferFragModule   = VK_NULL_HANDLE;
    VkShaderModule              lightingCompModule  = VK_NULL_HANDLE;
    VkShaderModule              refractionCompModule = VK_NULL_HANDLE;
    VkShaderModule              tonemapCompModule   = VK_NULL_HANDLE;

    // ⚙️ Viewport dimensions
    uint32_t                    viewportWidth  = 0;
    uint32_t                    viewportHeight = 0;
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace FilamentPipeline
{
    // ① Initialize the full render pipeline
    void constructPipeline(const VkBootstrapContext& ctx,
                           VkCommandPool             cmdPool,
                           FilamentPipelineState&    state,
                           uint32_t                  width,
                           uint32_t                  height,
                           const std::string&        shaderDir) noexcept(false);

    // ② Record and submit a frame
    void recordFrame(const VkBootstrapContext& ctx,
                     VkCommandBuffer           cmd,
                     FilamentPipelineState&    state,
                     const FilamentCamera&     camera,
                     const std::vector<SceneObject>& objects,
                     float                     timeSeconds) noexcept;

    // ③ Rebuild size-dependent resources
    void resizePipeline(const VkBootstrapContext& ctx,
                        VkCommandPool             cmdPool,
                        FilamentPipelineState&    state,
                        uint32_t                  width,
                        uint32_t                  height) noexcept(false);

    // ④ Retire the pipeline
    void retirePipeline(VkDevice device, FilamentPipelineState& state) noexcept;
}
