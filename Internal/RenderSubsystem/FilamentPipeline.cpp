// File: Source/Filament/Internal/RenderSubsystem/FilamentPipeline.cpp

/*====================================================================================================================================
                                                   FILAMENTPIPELINE.CPP
====================================================================================================================================*/
// 🧩 Full frame orchestrator: init, record, resize, retire.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "FilamentPipeline.h"
#include "../Algebra/Matrix.h"
#include "../Algebra/Utilities.h"
#include "../Auxiliary/FilamentLog.h"

#include <cstring>
#include <cmath>
#include <algorithm>

namespace FilamentPipeline
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  HELPER: UPLOAD SCENE UBO
//------------------------------------------------------------------------------------------------------------------------

static void writeSceneUbo(FilamentPipelineState& state,
                         const FilamentCamera&   camera,
                         float                   timeSeconds) noexcept
{
    float aspect = (state.viewportHeight > 0) ?
        static_cast<float>(state.viewportWidth) / static_cast<float>(state.viewportHeight) : 1.0f;

    Mat4 view = camera.queryViewMatrix();
    Mat4 proj = camera.queryProjectionMatrix(aspect);
    Mat4 invView = inverse(view);

    SceneUBO ubo = {};
    std::memcpy(ubo.viewMatrix,    &view,    64);
    std::memcpy(ubo.projMatrix,    &proj,    64);
    std::memcpy(ubo.invViewMatrix, &invView, 64);

    // ⚙️ Directional light from above-right
    Vec3 lightDir = normalize(Vec3(0.4f, -0.8f, -0.3f));
    ubo.lightDirection[0] = lightDir.x;
    ubo.lightDirection[1] = lightDir.y;
    ubo.lightDirection[2] = lightDir.z;
    ubo.lightDirection[3] = 0.0f;

    ubo.lightColor[0] = 1.0f;
    ubo.lightColor[1] = 0.98f;
    ubo.lightColor[2] = 0.95f;
    ubo.lightColor[3] = 3.0f;                   // [cd] - Light intensity

    Vec3 camPos = camera.queryPosition();
    ubo.cameraPosition[0] = camPos.x;
    ubo.cameraPosition[1] = camPos.y;
    ubo.cameraPosition[2] = camPos.z;
    ubo.cameraPosition[3] = 0.0f;

    ubo.timeAndResolution[0] = timeSeconds;
    ubo.timeAndResolution[1] = static_cast<float>(state.viewportWidth);
    ubo.timeAndResolution[2] = static_cast<float>(state.viewportHeight);
    ubo.timeAndResolution[3] = 0.0f;

    std::memcpy(state.sceneUboBuffer.mapped, &ubo, sizeof(SceneUBO));
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  HELPER: UPLOAD MATERIAL UBO
//------------------------------------------------------------------------------------------------------------------------

static void writeMaterialUbo(GpuBuffer& buffer, const FilamentMaterial& mat) noexcept
{
    std::memcpy(buffer.mapped, &mat, sizeof(FilamentMaterial));
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  PIPELINE CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructPipeline(const VkBootstrapContext& ctx,
                       VkCommandPool             cmdPool,
                       FilamentPipelineState&    state,
                       uint32_t                  width,
                       uint32_t                  height,
                       const std::string&        shaderDir) noexcept(false)
{
    state.viewportWidth  = width;
    state.viewportHeight = height;

    // ① Construct GBuffer
    FilamentGBuffer::constructGBuffer(ctx, state.gbuffer, width, height);

    // ② Construct offscreen images (for compute passes)
    VkMemory::allocateImage2D(ctx, width, height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        state.litColorImage);

    VkMemory::allocateImage2D(ctx, width, height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        state.refractionImage);

    VkMemory::allocateImage2D(ctx, width, height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        state.tonemappedImage);

    // ③ Sampler
    state.linearSampler = VkMemory::constructLinearSampler(ctx.device);

    // ④ Descriptor layouts
    VkDescriptorBank::constructLayouts(ctx.device, state.descLayouts);

    // ⑤ Pipeline layouts
    state.gbufferPipeLayout = VkDescriptorBank::constructGBufferPipelineLayout(ctx.device, state.descLayouts);
    state.computePipeLayout = VkDescriptorBank::constructComputePipelineLayout(ctx.device, state.descLayouts);

    // ⑥ Load shaders
    state.gbufferVertModule    = VkPipelineForge::forgeShaderModule(ctx.device, shaderDir + "/filament_gbuffer_vert.spv");
    state.gbufferFragModule    = VkPipelineForge::forgeShaderModule(ctx.device, shaderDir + "/filament_gbuffer_frag.spv");
    state.lightingCompModule   = VkPipelineForge::forgeShaderModule(ctx.device, shaderDir + "/filament_lighting.spv");
    state.refractionCompModule = VkPipelineForge::forgeShaderModule(ctx.device, shaderDir + "/filament_refraction.spv");
    state.tonemapCompModule    = VkPipelineForge::forgeShaderModule(ctx.device, shaderDir + "/filament_tonemap.spv");

    // ⑦ Create pipelines
    state.gbufferPipeline = VkPipelineForge::forgeGBufferPipeline(
        ctx.device, state.gbufferPipeLayout, state.gbuffer.renderPass, 0,
        state.gbufferVertModule, state.gbufferFragModule, FILAMENT_GBUFFER_RT_COUNT);

    state.lightingPipeline   = VkPipelineForge::forgeComputePipeline(ctx.device, state.computePipeLayout, state.lightingCompModule);
    state.refractionPipeline = VkPipelineForge::forgeComputePipeline(ctx.device, state.computePipeLayout, state.refractionCompModule);
    state.tonemapPipeline    = VkPipelineForge::forgeComputePipeline(ctx.device, state.computePipeLayout, state.tonemapCompModule);

    // ⑧ Scene UBO
    VkMemory::allocateBuffer(ctx, sizeof(SceneUBO),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        state.sceneUboBuffer);

    // ⑨ Load default scene
    std::vector<MeshData>   meshData;
    std::vector<SceneObject> sceneObjects;
    FilamentSceneBuilder::populateDefaultScene(meshData, sceneObjects);

    // ⑩ Upload meshes to GPU
    for (const auto& mesh : meshData)
    {
        GpuBuffer vb, ib;
        VkDeviceSize vbSize = mesh.vertices.size() * sizeof(FilamentVertex);
        VkDeviceSize ibSize = mesh.indices.size()  * sizeof(uint32_t);

        VkMemory::allocateBuffer(ctx, vbSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vb);
        VkMemory::uploadToDeviceBuffer(ctx, cmdPool, mesh.vertices.data(), vbSize, vb);

        VkMemory::allocateBuffer(ctx, ibSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ib);
        VkMemory::uploadToDeviceBuffer(ctx, cmdPool, mesh.indices.data(), ibSize, ib);

        state.vertexBuffers.push_back(vb);
        state.indexBuffers.push_back(ib);
        state.indexCounts.push_back(static_cast<uint32_t>(mesh.indices.size()));
    }

    // ⑪ Material UBOs
    for (const auto& obj : sceneObjects)
    {
        GpuBuffer matBuf;
        VkMemory::allocateBuffer(ctx, sizeof(FilamentMaterial),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            matBuf);
        writeMaterialUbo(matBuf, obj.material);
        state.materialUboBuffers.push_back(matBuf);
    }

    // ⑫ Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         32 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          16 }
    };
    state.descriptorPool = VkDescriptorBank::constructPool(ctx.device, 64, poolSizes);

    // ⑬ Allocate and write descriptor sets
    // Scene descriptor set
    VkDescriptorBank::allocateSets(ctx.device, state.descriptorPool,
        &state.descLayouts.sceneLayout, 1, &state.sceneDescSet);
    VkDescriptorBank::writeBufferDescriptor(ctx.device, state.sceneDescSet, 0,
        state.sceneUboBuffer.buffer, 0, sizeof(SceneUBO));

    // Material descriptor sets
    state.materialDescSets.resize(sceneObjects.size());
    for (size_t i = 0; i < sceneObjects.size(); ++i)
    {
        VkDescriptorBank::allocateSets(ctx.device, state.descriptorPool,
            &state.descLayouts.materialLayout, 1, &state.materialDescSets[i]);
        VkDescriptorBank::writeBufferDescriptor(ctx.device, state.materialDescSets[i], 0,
            state.materialUboBuffers[i].buffer, 0, sizeof(FilamentMaterial));
    }

    // Lighting compute descriptor set  (reads GBuffer RTs + depth, writes litColorImage)
    VkDescriptorBank::allocateSets(ctx.device, state.descriptorPool,
        &state.descLayouts.computeLayout, 1, &state.lightingDescSet);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.lightingDescSet, 0,
        state.gbuffer.colorTargets[0].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.lightingDescSet, 1,
        state.gbuffer.colorTargets[1].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.lightingDescSet, 2,
        state.gbuffer.colorTargets[2].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.lightingDescSet, 3,
        state.gbuffer.depthTarget.view, state.linearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkDescriptorBank::writeStorageImageDescriptor(ctx.device, state.lightingDescSet, 4,
        state.litColorImage.view);
    VkDescriptorBank::writeBufferDescriptor(ctx.device, state.lightingDescSet, 5,
        state.sceneUboBuffer.buffer, 0, sizeof(SceneUBO));

    // Refraction compute descriptor set (reads litColor + GBuffer normals, writes refractionImage)
    VkDescriptorBank::allocateSets(ctx.device, state.descriptorPool,
        &state.descLayouts.computeLayout, 1, &state.refractionDescSet);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.refractionDescSet, 0,
        state.litColorImage.view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.refractionDescSet, 1,
        state.gbuffer.colorTargets[1].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.refractionDescSet, 2,
        state.gbuffer.colorTargets[2].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.refractionDescSet, 3,
        state.gbuffer.depthTarget.view, state.linearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkDescriptorBank::writeStorageImageDescriptor(ctx.device, state.refractionDescSet, 4,
        state.refractionImage.view);
    VkDescriptorBank::writeBufferDescriptor(ctx.device, state.refractionDescSet, 5,
        state.sceneUboBuffer.buffer, 0, sizeof(SceneUBO));

    // Tonemap compute descriptor set (reads refractionImage, writes tonemappedImage)
    VkDescriptorBank::allocateSets(ctx.device, state.descriptorPool,
        &state.descLayouts.computeLayout, 1, &state.tonemapDescSet);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.tonemapDescSet, 0,
        state.refractionImage.view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.tonemapDescSet, 1,
        state.gbuffer.colorTargets[1].view, state.linearSampler); // unused but bound
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.tonemapDescSet, 2,
        state.gbuffer.colorTargets[2].view, state.linearSampler); // unused but bound
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.tonemapDescSet, 3,
        state.gbuffer.depthTarget.view, state.linearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); // unused but bound
    VkDescriptorBank::writeStorageImageDescriptor(ctx.device, state.tonemapDescSet, 4,
        state.tonemappedImage.view);
    VkDescriptorBank::writeBufferDescriptor(ctx.device, state.tonemapDescSet, 5,
        state.sceneUboBuffer.buffer, 0, sizeof(SceneUBO));

    FilamentLog::info("Render pipeline constructed: %u objects, %u meshes.", (uint32_t)sceneObjects.size(), (uint32_t)meshData.size());
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  RECORD FRAME
//------------------------------------------------------------------------------------------------------------------------

void recordFrame(const VkBootstrapContext& ctx,
                 VkCommandBuffer           cmd,
                 FilamentPipelineState&    state,
                 const FilamentCamera&     camera,
                 const std::vector<SceneObject>& objects,
                 float                     timeSeconds) noexcept
{
    // ① Update scene UBO
    writeSceneUbo(state, camera, timeSeconds);

    // ② Update material UBOs
    for (size_t i = 0; i < objects.size() && i < state.materialUboBuffers.size(); ++i)
    {
        writeMaterialUbo(state.materialUboBuffers[i], objects[i].material);
    }

    uint32_t w = state.viewportWidth;
    uint32_t h = state.viewportHeight;

    // ③ GBuffer pass
    {
        VkClearValue clearValues[4] = {};
        clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[1].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[2].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[3].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpBegin = {};
        rpBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass        = state.gbuffer.renderPass;
        rpBegin.framebuffer       = state.gbuffer.framebuffer;
        rpBegin.renderArea.extent = { w, h };
        rpBegin.clearValueCount   = 4;
        rpBegin.pClearValues      = clearValues;

        vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = { 0, 0, (float)w, (float)h, 0.0f, 1.0f };
        VkRect2D   scissor  = { {0, 0}, {w, h} };
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.gbufferPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.gbufferPipeLayout,
            0, 1, &state.sceneDescSet, 0, nullptr);

        for (size_t i = 0; i < objects.size(); ++i)
        {
            const auto& obj = objects[i];

            // ⚙️ Compute model and normal matrix
            Mat4 model = translationMatrix(obj.position) *
                         rotationMatrix(Vec3(0,1,0), obj.rotation.x) *
                         rotationMatrix(Vec3(1,0,0), obj.rotation.y) *
                         rotationMatrix(Vec3(0,0,1), obj.rotation.z) *
                         scaleMatrix(obj.scale);
            Mat3 normalMat3 = transpose(inverse(model.extractMat3()));
            // ⚙️ Pack normalMat3 into a Mat4 for push constant (padding)
            Mat4 normalMat4;
            normalMat4[0] = Vec4(normalMat3[0], 0.0f);
            normalMat4[1] = Vec4(normalMat3[1], 0.0f);
            normalMat4[2] = Vec4(normalMat3[2], 0.0f);
            normalMat4[3] = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

            ModelPushConstant pc;
            std::memcpy(pc.modelMatrix,  &model,     64);
            std::memcpy(pc.normalMatrix, &normalMat4, 64);

            vkCmdPushConstants(cmd, state.gbufferPipeLayout,
                VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelPushConstant), &pc);

            if (i < state.materialDescSets.size())
            {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.gbufferPipeLayout,
                    1, 1, &state.materialDescSets[i], 0, nullptr);
            }

            uint32_t meshIdx = obj.meshIndex;
            if (meshIdx < state.vertexBuffers.size())
            {
                VkBuffer     vbs[]    = { state.vertexBuffers[meshIdx].buffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(cmd, 0, 1, vbs, offsets);
                vkCmdBindIndexBuffer(cmd, state.indexBuffers[meshIdx].buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, state.indexCounts[meshIdx], 1, 0, 0, 0);
            }
        }

        vkCmdEndRenderPass(cmd);
    }

    // ⚙️ Compute workgroup dimensions (16x16 tiles)
    uint32_t groupsX = (w + 15) / 16;
    uint32_t groupsY = (h + 15) / 16;

    // ④ Transition offscreen images to general for compute writes
    VkSync::transitionImageLayout(cmd, state.litColorImage.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // ⑤ Lighting compute dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state.lightingPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state.computePipeLayout,
        0, 1, &state.lightingDescSet, 0, nullptr);
    vkCmdDispatch(cmd, groupsX, groupsY, 1);

    // ⑥ Barrier: lighting output → refraction input
    VkSync::transitionImageLayout(cmd, state.litColorImage.image,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkSync::transitionImageLayout(cmd, state.refractionImage.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // ⑦ Refraction compute dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state.refractionPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state.computePipeLayout,
        0, 1, &state.refractionDescSet, 0, nullptr);
    vkCmdDispatch(cmd, groupsX, groupsY, 1);

    // ⑧ Barrier: refraction output → tonemap input
    VkSync::transitionImageLayout(cmd, state.refractionImage.image,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkSync::transitionImageLayout(cmd, state.tonemappedImage.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // ⑨ Tonemap compute dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state.tonemapPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state.computePipeLayout,
        0, 1, &state.tonemapDescSet, 0, nullptr);
    vkCmdDispatch(cmd, groupsX, groupsY, 1);

    // ⑩ Barrier: tonemap output → shader read (for ImGui viewport)
    VkSync::transitionImageLayout(cmd, state.tonemappedImage.image,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  RESIZE
//------------------------------------------------------------------------------------------------------------------------

void resizePipeline(const VkBootstrapContext& ctx,
                    VkCommandPool             cmdPool,
                    FilamentPipelineState&    state,
                    uint32_t                  width,
                    uint32_t                  height) noexcept(false)
{
    vkDeviceWaitIdle(ctx.device);

    state.viewportWidth  = width;
    state.viewportHeight = height;

    // ⚙️ Recreate size-dependent images
    FilamentGBuffer::recreateGBuffer(ctx, state.gbuffer, width, height);

    VkMemory::retireImage(ctx.device, state.litColorImage);
    VkMemory::retireImage(ctx.device, state.refractionImage);
    VkMemory::retireImage(ctx.device, state.tonemappedImage);

    VkMemory::allocateImage2D(ctx, width, height, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, state.litColorImage);
    VkMemory::allocateImage2D(ctx, width, height, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, state.refractionImage);
    VkMemory::allocateImage2D(ctx, width, height, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, state.tonemappedImage);

    // ⚙️ Update compute descriptor sets with new image views
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.lightingDescSet, 0,
        state.gbuffer.colorTargets[0].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.lightingDescSet, 1,
        state.gbuffer.colorTargets[1].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.lightingDescSet, 2,
        state.gbuffer.colorTargets[2].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.lightingDescSet, 3,
        state.gbuffer.depthTarget.view, state.linearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkDescriptorBank::writeStorageImageDescriptor(ctx.device, state.lightingDescSet, 4,
        state.litColorImage.view);

    VkDescriptorBank::writeImageDescriptor(ctx.device, state.refractionDescSet, 0,
        state.litColorImage.view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.refractionDescSet, 1,
        state.gbuffer.colorTargets[1].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.refractionDescSet, 2,
        state.gbuffer.colorTargets[2].view, state.linearSampler);
    VkDescriptorBank::writeImageDescriptor(ctx.device, state.refractionDescSet, 3,
        state.gbuffer.depthTarget.view, state.linearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkDescriptorBank::writeStorageImageDescriptor(ctx.device, state.refractionDescSet, 4,
        state.refractionImage.view);

    VkDescriptorBank::writeImageDescriptor(ctx.device, state.tonemapDescSet, 0,
        state.refractionImage.view, state.linearSampler);
    VkDescriptorBank::writeStorageImageDescriptor(ctx.device, state.tonemapDescSet, 4,
        state.tonemappedImage.view);

    FilamentLog::info("Pipeline resized: %ux%u.", width, height);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  PIPELINE RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void retirePipeline(VkDevice device, FilamentPipelineState& state) noexcept
{
    // ⚙️ Pipelines
    if (state.gbufferPipeline)    vkDestroyPipeline(device, state.gbufferPipeline, nullptr);
    if (state.lightingPipeline)   vkDestroyPipeline(device, state.lightingPipeline, nullptr);
    if (state.refractionPipeline) vkDestroyPipeline(device, state.refractionPipeline, nullptr);
    if (state.tonemapPipeline)    vkDestroyPipeline(device, state.tonemapPipeline, nullptr);

    // ⚙️ Pipeline layouts
    if (state.gbufferPipeLayout) vkDestroyPipelineLayout(device, state.gbufferPipeLayout, nullptr);
    if (state.computePipeLayout) vkDestroyPipelineLayout(device, state.computePipeLayout, nullptr);

    // ⚙️ Shader modules
    if (state.gbufferVertModule)    vkDestroyShaderModule(device, state.gbufferVertModule, nullptr);
    if (state.gbufferFragModule)    vkDestroyShaderModule(device, state.gbufferFragModule, nullptr);
    if (state.lightingCompModule)   vkDestroyShaderModule(device, state.lightingCompModule, nullptr);
    if (state.refractionCompModule) vkDestroyShaderModule(device, state.refractionCompModule, nullptr);
    if (state.tonemapCompModule)    vkDestroyShaderModule(device, state.tonemapCompModule, nullptr);

    // ⚙️ Descriptors
    if (state.descriptorPool) vkDestroyDescriptorPool(device, state.descriptorPool, nullptr);
    VkDescriptorBank::retireLayouts(device, state.descLayouts);

    // ⚙️ Images
    VkMemory::retireImage(device, state.litColorImage);
    VkMemory::retireImage(device, state.refractionImage);
    VkMemory::retireImage(device, state.tonemappedImage);
    FilamentGBuffer::retireGBuffer(device, state.gbuffer);

    // ⚙️ Buffers
    VkMemory::retireBuffer(device, state.sceneUboBuffer);
    for (auto& buf : state.materialUboBuffers) VkMemory::retireBuffer(device, buf);
    for (auto& buf : state.vertexBuffers)      VkMemory::retireBuffer(device, buf);
    for (auto& buf : state.indexBuffers)       VkMemory::retireBuffer(device, buf);

    // ⚙️ Sampler
    if (state.linearSampler) vkDestroySampler(device, state.linearSampler, nullptr);

    state = {};
    FilamentLog::info("Render pipeline retired.");
}

} // namespace FilamentPipeline
