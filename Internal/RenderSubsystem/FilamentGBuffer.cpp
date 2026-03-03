// File: Source/Filament/Internal/RenderSubsystem/FilamentGBuffer.cpp

/*====================================================================================================================================
                                                   FILAMENTGBUFFER.CPP
====================================================================================================================================*/
// 🧩 GBuffer construction — render targets, render pass, framebuffer.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "FilamentGBuffer.h"
#include "../Auxiliary/FilamentTypes.h"
#include "../Auxiliary/FilamentLog.h"

namespace FilamentGBuffer
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  GBUFFER CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructGBuffer(const VkBootstrapContext& ctx,
                      FilamentGBufferState&     state,
                      uint32_t                  width,
                      uint32_t                  height) noexcept(false)
{
    state.width  = width;
    state.height = height;

    // ① Create render targets
    // RT0: Albedo.RGB + Metallic.A
    VkMemory::allocateImage2D(ctx, width, height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        state.colorTargets[0]);

    // RT1: Normal.XY (octahedral encoded, 16-bit float)
    VkMemory::allocateImage2D(ctx, width, height,
        VK_FORMAT_R16G16_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        state.colorTargets[1]);

    // RT2: Roughness.R + Specular.G
    VkMemory::allocateImage2D(ctx, width, height,
        VK_FORMAT_R8G8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        state.colorTargets[2]);

    // Depth: D32_SFLOAT
    VkMemory::allocateImage2D(ctx, width, height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        state.depthTarget);

    // ② Create render pass
    VkAttachmentDescription attachments[4] = {};

    // RT0
    attachments[0].format         = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // RT1
    attachments[1] = attachments[0];
    attachments[1].format = VK_FORMAT_R16G16_SFLOAT;

    // RT2
    attachments[2] = attachments[0];
    attachments[2].format = VK_FORMAT_R8G8_UNORM;

    // Depth
    attachments[3].format         = VK_FORMAT_D32_SFLOAT;
    attachments[3].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[3].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRefs[3] = {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
    };
    VkAttachmentReference depthRef = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 3;
    subpass.pColorAttachments       = colorRefs;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency deps[2] = {};
    deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass    = 0;
    deps[0].srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    deps[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[0].srcAccessMask = 0;
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    deps[1].srcSubpass    = 0;
    deps[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo rpInfo = {};
    rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 4;
    rpInfo.pAttachments    = attachments;
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = 2;
    rpInfo.pDependencies   = deps;

    FILAMENT_VK_VERIFY(vkCreateRenderPass(ctx.device, &rpInfo, nullptr, &state.renderPass));

    // ③ Create framebuffer
    VkImageView fbViews[] = {
        state.colorTargets[0].view,
        state.colorTargets[1].view,
        state.colorTargets[2].view,
        state.depthTarget.view
    };

    VkFramebufferCreateInfo fbInfo = {};
    fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass      = state.renderPass;
    fbInfo.attachmentCount = 4;
    fbInfo.pAttachments    = fbViews;
    fbInfo.width           = width;
    fbInfo.height          = height;
    fbInfo.layers          = 1;

    FILAMENT_VK_VERIFY(vkCreateFramebuffer(ctx.device, &fbInfo, nullptr, &state.framebuffer));

    FilamentLog::info("GBuffer constructed: %ux%u (3 RTs + depth).", width, height);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  GBUFFER RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void retireGBuffer(VkDevice device, FilamentGBufferState& state) noexcept
{
    if (state.framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device, state.framebuffer, nullptr);
    if (state.renderPass  != VK_NULL_HANDLE) vkDestroyRenderPass(device, state.renderPass, nullptr);

    for (auto& rt : state.colorTargets) VkMemory::retireImage(device, rt);
    VkMemory::retireImage(device, state.depthTarget);

    state = {};
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  GBUFFER RECREATION
//------------------------------------------------------------------------------------------------------------------------

void recreateGBuffer(const VkBootstrapContext& ctx,
                     FilamentGBufferState&     state,
                     uint32_t                  width,
                     uint32_t                  height) noexcept(false)
{
    retireGBuffer(ctx.device, state);
    constructGBuffer(ctx, state, width, height);
}

} // namespace FilamentGBuffer
