// File: Source/Filament/Internal/Interface/FilamentUI.h
#pragma once

/*====================================================================================================================================
                                                       FILAMENTUI.H
====================================================================================================================================*/
// 🧩 ImGui integration — viewport, outliner, object settings, render settings, FPS overlay.

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "../VulkanGraphicsInterface/Context/VkBootstrap.h"
#include "../VulkanGraphicsInterface/Resources/VkSwapchain.h"
#include "../VulkanGraphicsInterface/Descriptors/VkDescriptorBank.h"
#include "../VulkanGraphicsInterface/Memory/VkMemoryReservoir.h"
#include "../RenderSubsystem/FilamentPipeline.h"
#include "../SceneContext/FilamentCamera.h"
#include "../TextureSubsystem/FilamentMaterial.h"

#include <cstdint>
#include <vector>
#include <string>

//------------------------------------------------------------------------------------------------------------------------
//                                                    UI STATE
//------------------------------------------------------------------------------------------------------------------------

struct FilamentUIState
{
    VkDescriptorPool     imguiPool    = VK_NULL_HANDLE;  // [-] - ImGui descriptor pool
    VkRenderPass         imguiRenderPass = VK_NULL_HANDLE; // [-] - ImGui render pass
    std::vector<VkFramebuffer> imguiFramebuffers;         // [-] - Per-swapchain-image framebuffers
    VkDescriptorSet      viewportDescSet = VK_NULL_HANDLE; // [-] - Descriptor for viewport texture

    // ⚙️ UI state
    int                  selectedObject = 0;     // [idx] - Selected object in outliner
    bool                 showOutliner   = true;  // [-]   - Show outliner panel
    bool                 showSettings   = true;  // [-]   - Show object settings panel
    bool                 showRender     = true;  // [-]   - Show render settings panel
    bool                 showOverlay    = true;  // [-]   - Show FPS/telemetry overlay

    // ⚙️ Rendered frame timing
    float                frameTimeMs    = 0.0f;  // [ms]  - Last frame time
    float                fps            = 0.0f;  // [Hz]  - Frames per second
    float                fpsHistory[120] = {};    // [Hz]  - FPS history for graph
    int                  fpsHistoryIdx  = 0;     // [idx] - Circular buffer index
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace FilamentUI
{
    // ① Initialize ImGui with Vulkan backend
    void constructUI(const VkBootstrapContext& ctx,
                     const VkSwapchainState&  swapchain,
                     VkCommandPool            cmdPool,
                     FilamentUIState&         state,
                     HWND                     hwnd) noexcept(false);

    // ② Record ImGui draw commands for this frame
    void recordUI(const VkBootstrapContext&     ctx,
                  VkCommandBuffer               cmd,
                  FilamentUIState&              state,
                  FilamentPipelineState&        pipeline,
                  FilamentCamera&               camera,
                  std::vector<SceneObject>&     objects,
                  uint32_t                      swapchainImageIndex,
                  float                         deltaTime) noexcept;

    // ③ Rebuild framebuffers on swapchain recreation
    void rebuildFramebuffers(const VkBootstrapContext& ctx,
                             const VkSwapchainState&   swapchain,
                             FilamentUIState&          state) noexcept(false);

    // ④ Retire ImGui resources
    void retireUI(VkDevice device, FilamentUIState& state) noexcept;
}
