// File: Source/Filament/FilamentEntry.cpp

/*====================================================================================================================================
                                                     FILAMENTENTRY.CPP
====================================================================================================================================*/
// 🧩 Application entry point — Win32 window, Vulkan initialization, frame loop, shutdown.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Internal/Auxiliary/FilamentTypes.h"
#include "Internal/Auxiliary/FilamentLog.h"
#include "Internal/Algebra/Utilities.h"
#include "Internal/VulkanGraphicsInterface/Context/VkBootstrap.h"
#include "Internal/VulkanGraphicsInterface/Resources/VkSwapchain.h"
#include "Internal/VulkanGraphicsInterface/Commands/VkCommandDispatch.h"
#include "Internal/VulkanGraphicsInterface/Sync/VkSyncPrimitives.h"
#include "Internal/RenderSubsystem/FilamentPipeline.h"
#include "Internal/SceneContext/FilamentCamera.h"
#include "Internal/SceneContext/FilamentScene.h"
#include "Internal/Interface/FilamentUI.h"

// imgui includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

#include <chrono>
#include <string>

//------------------------------------------------------------------------------------------------------------------------
//                                                    GLOBALS
//------------------------------------------------------------------------------------------------------------------------

static bool                    gIsRunning         = true;
static bool                    gFramebufferResized = false;
static uint32_t                gWindowWidth       = 1600;  // [px]
static uint32_t                gWindowHeight      = 900;   // [px]

// ⚙️ Forward-declare ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


//------------------------------------------------------------------------------------------------------------------------
//                                                    WINDOW PROCEDURE
//------------------------------------------------------------------------------------------------------------------------

static LRESULT CALLBACK windowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return 1;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            gWindowWidth  = LOWORD(lParam);
            gWindowHeight = HIWORD(lParam);
            gFramebufferResized = true;
        }
        return 0;

    case WM_CLOSE:
    case WM_DESTROY:
        gIsRunning = false;
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            gIsRunning = false;
            PostQuitMessage(0);
        }
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    ENTRY POINT
//------------------------------------------------------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    try
    {
    return [&]() -> int
    {
    // ① Create Win32 window
    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = windowProcedure;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "FilamentWindowClass";
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(
        0, "FilamentWindowClass", "Filament — PBR Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(gWindowWidth), static_cast<int>(gWindowHeight),
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    FilamentLog::info("Window created: %ux%u.", gWindowWidth, gWindowHeight);

    // ② Initialize Vulkan
    VkBootstrapContext vkCtx;
    #ifdef NDEBUG
        bool enableValidation = false;
    #else
        bool enableValidation = true;
    #endif

    VkBootstrap::constructInstance(vkCtx, enableValidation);
    VkBootstrap::constructSurface(vkCtx, hwnd, hInstance);
    VkBootstrap::selectPhysicalDevice(vkCtx);
    VkBootstrap::constructDevice(vkCtx);

    // ③ Create swapchain
    VkSwapchainState swapchain;
    VkSwapchainOps::constructSwapchain(vkCtx, swapchain, gWindowWidth, gWindowHeight);

    // ④ Command pool + buffers
    VkCommandPool cmdPool;
    VkCommandDispatch::constructPool(vkCtx.device, vkCtx.graphicsFamily, cmdPool);

    std::vector<VkCommandBuffer> cmdBuffers;
    VkCommandDispatch::allocateBuffers(vkCtx.device, cmdPool, FILAMENT_MAX_FRAMES_IN_FLIGHT, cmdBuffers);

    // ⑤ Sync primitives
    FrameSyncPrimitives frameSync;
    VkSync::constructFrameSync(vkCtx.device, frameSync);

    // ⑥ Render pipeline
    FilamentPipelineState pipelineState;
    std::string shaderDir = "../../BuildArtifacts/Filament/Shaders";
    FilamentPipeline::constructPipeline(vkCtx, cmdPool, pipelineState,
        swapchain.extent.width, swapchain.extent.height, shaderDir);

    // ⑦ Scene objects (populated during pipeline construction, but also kept locally)
    std::vector<MeshData>    meshes;
    std::vector<SceneObject> sceneObjects;
    FilamentSceneBuilder::populateDefaultScene(meshes, sceneObjects);

    // ⑧ Camera
    FilamentCamera camera;
    camera.autoOrbit = true;

    // ⑨ ImGui
    FilamentUIState uiState;
    FilamentUI::constructUI(vkCtx, swapchain, cmdPool, uiState, hwnd);

    // ⑩ Frame loop
    uint32_t frameIndex = 0;
    auto     startTime  = std::chrono::high_resolution_clock::now();
    auto     lastFrame  = startTime;

    FilamentLog::info("Entering main loop.");

    while (gIsRunning)
    {
        // ⚙️ Process Win32 messages
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (!gIsRunning) break;

        // ⚙️ Delta time
        auto now       = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastFrame).count();
        float totalTime = std::chrono::duration<float>(now - startTime).count();
        lastFrame = now;

        // ⚙️ Skip minimized frames
        if (gWindowWidth == 0 || gWindowHeight == 0) continue;

        // ⚙️ Advance camera
        camera.advanceTime(deltaTime);

        // ⚙️ Wait for previous frame fence
        vkWaitForFences(vkCtx.device, 1, &frameSync.inFlightFence[frameIndex], VK_TRUE, UINT64_MAX);

        // ⚙️ Acquire swapchain image
        uint32_t imageIndex;
        VkResult acquireResult = vkAcquireNextImageKHR(
            vkCtx.device, swapchain.swapchain, UINT64_MAX,
            frameSync.imageAvailable[frameIndex], VK_NULL_HANDLE, &imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || gFramebufferResized)
        {
            gFramebufferResized = false;
            vkDeviceWaitIdle(vkCtx.device);
            VkSwapchainOps::recreateSwapchain(vkCtx, swapchain, gWindowWidth, gWindowHeight);
            FilamentUI::rebuildFramebuffers(vkCtx, swapchain, uiState);
            continue;
        }

        vkResetFences(vkCtx.device, 1, &frameSync.inFlightFence[frameIndex]);

        // ⚙️ Record command buffer
        VkCommandBuffer cmd = cmdBuffers[frameIndex];
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        // ⚙️ Record 3D rendering passes (GBuffer → Lighting → Refraction → Tonemap)
        FilamentPipeline::recordFrame(vkCtx, cmd, pipelineState, camera, sceneObjects, totalTime);

        // ⚙️ Transition swapchain image for ImGui rendering
        VkSync::transitionImageLayout(cmd, swapchain.images[imageIndex],
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // ⚙️ Record ImGui UI overlay (blits viewport + draws panels)
        FilamentUI::recordUI(vkCtx, cmd, uiState, pipelineState, camera, sceneObjects, imageIndex, deltaTime);

        // ⚙️ End command buffer
        vkEndCommandBuffer(cmd);

        // ⚙️ Submit
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &frameSync.imageAvailable[frameIndex];
        submitInfo.pWaitDstStageMask    = &waitStage;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &cmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &frameSync.renderFinished[frameIndex];

        vkQueueSubmit(vkCtx.graphicsQueue, 1, &submitInfo, frameSync.inFlightFence[frameIndex]);

        // ⚙️ Present
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &frameSync.renderFinished[frameIndex];
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &swapchain.swapchain;
        presentInfo.pImageIndices      = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(vkCtx.graphicsQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            gFramebufferResized = true;
        }

        frameIndex = (frameIndex + 1) % FILAMENT_MAX_FRAMES_IN_FLIGHT;
    }

    // ⑪ Shutdown
    FilamentLog::info("Shutting down...");
    vkDeviceWaitIdle(vkCtx.device);

    FilamentUI::retireUI(vkCtx.device, uiState);
    FilamentPipeline::retirePipeline(vkCtx.device, pipelineState);
    VkSync::retireFrameSync(vkCtx.device, frameSync);
    VkCommandDispatch::retirePool(vkCtx.device, cmdPool);
    VkSwapchainOps::retireSwapchain(vkCtx.device, swapchain);
    VkBootstrap::retireContext(vkCtx);

    DestroyWindow(hwnd);
    UnregisterClassA("FilamentWindowClass", hInstance);

    FilamentLog::info("Filament shut down cleanly.");
    return 0;
    }(); // end lambda
    }
    catch (const std::exception& e)
    {
        FilamentLog::error("Fatal: %s", e.what());
        MessageBoxA(nullptr, e.what(), "Filament — Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}
