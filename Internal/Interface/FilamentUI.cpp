// File: Source/Filament/Internal/Interface/FilamentUI.cpp

/*====================================================================================================================================
                                                      FILAMENTUI.CPP
====================================================================================================================================*/
// 🧩 ImGui integration — Vulkan backend initialization, panel rendering, viewport display.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "FilamentUI.h"
#include "../Auxiliary/FilamentLog.h"
#include "../Auxiliary/FilamentTypes.h"

// imgui include here (replaced later)
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

namespace FilamentUI
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  UI CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructUI(const VkBootstrapContext& ctx,
                 const VkSwapchainState&  swapchain,
                 VkCommandPool            cmdPool,
                 FilamentUIState&         state,
                 HWND                     hwnd) noexcept(false)
{
    // ① Create ImGui descriptor pool
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = 16;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = poolSizes;

    FILAMENT_VK_VERIFY(vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &state.imguiPool));

    // ② Create ImGui render pass (single color attachment, loads existing content)
    VkAttachmentDescription attachment = {};
    attachment.format         = swapchain.imageFormat;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorRef;

    VkSubpassDependency dep = {};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpInfo = {};
    rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments    = &attachment;
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies   = &dep;

    FILAMENT_VK_VERIFY(vkCreateRenderPass(ctx.device, &rpInfo, nullptr, &state.imguiRenderPass));

    // ③ Create framebuffers (one per swapchain image)
    state.imguiFramebuffers.resize(swapchain.imageCount);
    for (uint32_t i = 0; i < swapchain.imageCount; ++i)
    {
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass      = state.imguiRenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments    = &swapchain.imageViews[i];
        fbInfo.width           = swapchain.extent.width;
        fbInfo.height          = swapchain.extent.height;
        fbInfo.layers          = 1;

        FILAMENT_VK_VERIFY(vkCreateFramebuffer(ctx.device, &fbInfo, nullptr, &state.imguiFramebuffers[i]));
    }

    // ④ Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hwnd);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 6.0f;
    style.FrameRounding     = 4.0f;
    style.GrabRounding      = 3.0f;
    style.ScrollbarSize     = 12.0f;
    style.WindowBorderSize  = 1.0f;
    style.Colors[ImGuiCol_WindowBg]       = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    style.Colors[ImGuiCol_TitleBg]        = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_Button]         = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]  = ImVec4(0.28f, 0.28f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.20f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.26f, 0.26f, 0.36f, 1.00f);

    // ⑤ Initialize ImGui platform/renderer backends
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance       = ctx.instance;
    initInfo.PhysicalDevice = ctx.physicalDevice;
    initInfo.Device         = ctx.device;
    initInfo.QueueFamily    = ctx.graphicsFamily;
    initInfo.Queue          = ctx.graphicsQueue;
    initInfo.DescriptorPool = state.imguiPool;
    initInfo.MinImageCount  = swapchain.imageCount;
    initInfo.ImageCount     = swapchain.imageCount;
    initInfo.PipelineInfoMain.RenderPass = state.imguiRenderPass;
    initInfo.PipelineInfoMain.Subpass    = 0;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.UseDynamicRendering = false;

    ImGui_ImplVulkan_Init(&initInfo);

    FilamentLog::info("ImGui UI constructed.");
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  RECORD UI
//------------------------------------------------------------------------------------------------------------------------

void recordUI(const VkBootstrapContext&     ctx,
              VkCommandBuffer               cmd,
              FilamentUIState&              state,
              FilamentPipelineState&        pipeline,
              FilamentCamera&               camera,
              std::vector<SceneObject>&     objects,
              uint32_t                      swapchainImageIndex,
              float                         deltaTime) noexcept
{
    // ① Begin ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // ② Update timing
    state.frameTimeMs = deltaTime * 1000.0f;
    state.fps         = (deltaTime > 0.0f) ? 1.0f / deltaTime : 0.0f;
    state.fpsHistory[state.fpsHistoryIdx] = state.fps;
    state.fpsHistoryIdx = (state.fpsHistoryIdx + 1) % 120;

    // ③ Viewport window — displays the offscreen tonemapped image
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImVec2 viewSize = ImGui::GetContentRegionAvail();

        // ⚙️ Resize pipeline if viewport size changed
        uint32_t vw = static_cast<uint32_t>(viewSize.x);
        uint32_t vh = static_cast<uint32_t>(viewSize.y);
        if (vw > 0 && vh > 0 && (vw != pipeline.viewportWidth || vh != pipeline.viewportHeight))
        {
            FilamentPipeline::resizePipeline(ctx, VK_NULL_HANDLE, pipeline, vw, vh);

            // ⚙️ Update viewport descriptor set
            if (state.viewportDescSet != VK_NULL_HANDLE)
            {
                ImGui_ImplVulkan_RemoveTexture(state.viewportDescSet);
            }
            state.viewportDescSet = ImGui_ImplVulkan_AddTexture(
                pipeline.linearSampler,
                pipeline.tonemappedImage.view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        if (state.viewportDescSet == VK_NULL_HANDLE && pipeline.tonemappedImage.view != VK_NULL_HANDLE)
        {
            state.viewportDescSet = ImGui_ImplVulkan_AddTexture(
                pipeline.linearSampler,
                pipeline.tonemappedImage.view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        if (state.viewportDescSet)
        {
            ImGui::Image((ImTextureID)state.viewportDescSet, viewSize);
        }
    }
    ImGui::End();

    // ④ Outliner panel
    if (state.showOutliner)
    {
        ImGui::Begin("Outliner", &state.showOutliner);
        for (int i = 0; i < static_cast<int>(objects.size()); ++i)
        {
            bool selected = (state.selectedObject == i);
            if (ImGui::Selectable(objects[i].name.c_str(), selected))
            {
                state.selectedObject = i;
            }
        }
        ImGui::End();
    }

    // ⑤ Object Settings panel
    if (state.showSettings && state.selectedObject >= 0 && state.selectedObject < static_cast<int>(objects.size()))
    {
        auto& obj = objects[state.selectedObject];

        ImGui::Begin("Object Settings", &state.showSettings);

        ImGui::Text("Name: %s", obj.name.c_str());
        ImGui::Separator();

        // ⚙️ Transform
        ImGui::DragFloat3("Position", &obj.position.x, 0.1f);
        ImGui::DragFloat3("Scale",    &obj.scale.x,    0.01f);
        ImGui::DragFloat3("Rotation", &obj.rotation.x, 0.01f);
        ImGui::Separator();

        // ⚙️ PBR Material
        ImGui::ColorEdit3("Albedo",    &obj.material.albedo.x);
        ImGui::SliderFloat("Roughness", &obj.material.roughness,        0.01f, 1.0f);
        ImGui::SliderFloat("Metallic",  &obj.material.metallic,         0.0f,  1.0f);
        ImGui::SliderFloat("F0",        &obj.material.specularF0,       0.0f,  1.0f);
        ImGui::SliderFloat("Refraction", &obj.material.refractionStrength, 0.0f, 1.0f);

        ImGui::End();
    }

    // ⑥ Render Settings panel
    if (state.showRender)
    {
        ImGui::Begin("Render Settings", &state.showRender);

        ImGui::Text("Camera");
        ImGui::Checkbox("Auto Orbit", &camera.autoOrbit);
        ImGui::SliderFloat("Orbit Speed",  &camera.orbitSpeed, 0.0f, 2.0f);
        ImGui::SliderFloat("Azimuth",      &camera.azimuth,    0.0f, 6.28318f);
        ImGui::SliderFloat("Elevation",    &camera.elevation,  -1.5f, 1.5f);
        ImGui::SliderFloat("Radius",       &camera.radius,     1.0f, 50.0f);
        ImGui::SliderFloat("FOV",          &camera.fovY,       0.2f, 2.5f);

        ImGui::End();
    }

    // ⑦ FPS/Telemetry overlay
    if (state.showOverlay)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.6f);
        ImGui::Begin("Telemetry", &state.showOverlay,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

        ImGui::Text("FPS: %.1f", state.fps);
        ImGui::Text("Frame: %.2f ms", state.frameTimeMs);
        ImGui::Text("Viewport: %ux%u", pipeline.viewportWidth, pipeline.viewportHeight);
        ImGui::PlotLines("##fps", state.fpsHistory, 120, state.fpsHistoryIdx,
                         nullptr, 0.0f, 120.0f, ImVec2(200, 40));

        ImGui::End();
    }

    // ⑧ Render ImGui draw data
    ImGui::Render();

    VkRenderPassBeginInfo rpBegin = {};
    rpBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass        = state.imguiRenderPass;
    rpBegin.framebuffer       = state.imguiFramebuffers[swapchainImageIndex];
    rpBegin.renderArea.extent = { pipeline.viewportWidth, pipeline.viewportHeight };

    // ⚙️ Use largest available extent from swapchain framebuffer
    VkFramebufferCreateInfo fbQuery = {};
    // Render area set to swapchain extent is handled by the framebuffer dimensions

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRenderPass(cmd);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  REBUILD FRAMEBUFFERS
//------------------------------------------------------------------------------------------------------------------------

void rebuildFramebuffers(const VkBootstrapContext& ctx,
                         const VkSwapchainState&   swapchain,
                         FilamentUIState&          state) noexcept(false)
{
    for (auto fb : state.imguiFramebuffers)
    {
        if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(ctx.device, fb, nullptr);
    }

    state.imguiFramebuffers.resize(swapchain.imageCount);
    for (uint32_t i = 0; i < swapchain.imageCount; ++i)
    {
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass      = state.imguiRenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments    = &swapchain.imageViews[i];
        fbInfo.width           = swapchain.extent.width;
        fbInfo.height          = swapchain.extent.height;
        fbInfo.layers          = 1;

        FILAMENT_VK_VERIFY(vkCreateFramebuffer(ctx.device, &fbInfo, nullptr, &state.imguiFramebuffers[i]));
    }
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  UI RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void retireUI(VkDevice device, FilamentUIState& state) noexcept
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    for (auto fb : state.imguiFramebuffers)
    {
        if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(device, fb, nullptr);
    }

    if (state.imguiRenderPass != VK_NULL_HANDLE) vkDestroyRenderPass(device, state.imguiRenderPass, nullptr);
    if (state.imguiPool       != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, state.imguiPool, nullptr);

    state = {};
    FilamentLog::info("ImGui UI retired.");
}

} // namespace FilamentUI
