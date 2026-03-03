// File: Source/Filament/Internal/VulkanGraphicsInterface/Descriptors/VkDescriptorBank.cpp

/*====================================================================================================================================
                                                   VKDESCRIPTORBANK.CPP
====================================================================================================================================*/
// 🧩 Descriptor layout, pool, allocation, and write implementation.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VkDescriptorBank.h"
#include "../../Auxiliary/FilamentTypes.h"

namespace VkDescriptorBank
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  LAYOUT CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void constructLayouts(VkDevice device, FilamentDescriptorLayouts& layouts) noexcept(false)
{
    // ① Set 0: Scene UBO (binding 0)
    {
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding         = 0;
        binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings    = &binding;

        FILAMENT_VK_VERIFY(vkCreateDescriptorSetLayout(device, &info, nullptr, &layouts.sceneLayout));
    }

    // ② Set 1: Material UBO (binding 0)
    {
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding         = 0;
        binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings    = &binding;

        FILAMENT_VK_VERIFY(vkCreateDescriptorSetLayout(device, &info, nullptr, &layouts.materialLayout));
    }

    // ③ Set 2: GBuffer images for lighting pass (4 combined image samplers: RT0, RT1, RT2, Depth)
    {
        VkDescriptorSetLayoutBinding bindings[4] = {};
        for (uint32_t i = 0; i < 4; ++i)
        {
            bindings[i].binding         = i;
            bindings[i].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 4;
        info.pBindings    = bindings;

        FILAMENT_VK_VERIFY(vkCreateDescriptorSetLayout(device, &info, nullptr, &layouts.gbufferLayout));
    }

    // ④ Compute pass layout: binding 0-3 = input images, binding 4 = output storage image, binding 5 = scene UBO
    {
        VkDescriptorSetLayoutBinding bindings[6] = {};
        // Input samplers
        for (uint32_t i = 0; i < 4; ++i)
        {
            bindings[i].binding         = i;
            bindings[i].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        // Output storage image
        bindings[4].binding         = 4;
        bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        // Scene UBO
        bindings[5].binding         = 5;
        bindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[5].descriptorCount = 1;
        bindings[5].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 6;
        info.pBindings    = bindings;

        FILAMENT_VK_VERIFY(vkCreateDescriptorSetLayout(device, &info, nullptr, &layouts.computeLayout));
    }
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  LAYOUT RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

void retireLayouts(VkDevice device, FilamentDescriptorLayouts& layouts) noexcept
{
    if (layouts.sceneLayout    != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, layouts.sceneLayout, nullptr);
    if (layouts.materialLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, layouts.materialLayout, nullptr);
    if (layouts.gbufferLayout  != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, layouts.gbufferLayout, nullptr);
    if (layouts.computeLayout  != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, layouts.computeLayout, nullptr);
    layouts = {};
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  POOL CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

VkDescriptorPool constructPool(VkDevice device,
                                uint32_t maxSets,
                                const std::vector<VkDescriptorPoolSize>& sizes) noexcept(false)
{
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets       = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
    poolInfo.pPoolSizes    = sizes.data();

    VkDescriptorPool pool;
    FILAMENT_VK_VERIFY(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool));
    return pool;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  SET ALLOCATION
//------------------------------------------------------------------------------------------------------------------------

void allocateSets(VkDevice              device,
                  VkDescriptorPool      pool,
                  const VkDescriptorSetLayout* layouts,
                  uint32_t              count,
                  VkDescriptorSet*      outSets) noexcept(false)
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = pool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts        = layouts;

    FILAMENT_VK_VERIFY(vkAllocateDescriptorSets(device, &allocInfo, outSets));
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  DESCRIPTOR WRITES
//------------------------------------------------------------------------------------------------------------------------

void writeBufferDescriptor(VkDevice         device,
                           VkDescriptorSet  dstSet,
                           uint32_t         binding,
                           VkBuffer         buffer,
                           VkDeviceSize     offset,
                           VkDeviceSize     range) noexcept
{
    VkDescriptorBufferInfo bufInfo = {};
    bufInfo.buffer = buffer;
    bufInfo.offset = offset;
    bufInfo.range  = range;

    VkWriteDescriptorSet write = {};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = dstSet;
    write.dstBinding      = binding;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo     = &bufInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void writeImageDescriptor(VkDevice         device,
                          VkDescriptorSet  dstSet,
                          uint32_t         binding,
                          VkImageView      imageView,
                          VkSampler        sampler,
                          VkImageLayout    layout) noexcept
{
    VkDescriptorImageInfo imgInfo = {};
    imgInfo.imageView   = imageView;
    imgInfo.sampler     = sampler;
    imgInfo.imageLayout = layout;

    VkWriteDescriptorSet write = {};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = dstSet;
    write.dstBinding      = binding;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo      = &imgInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void writeStorageImageDescriptor(VkDevice         device,
                                 VkDescriptorSet  dstSet,
                                 uint32_t         binding,
                                 VkImageView      imageView) noexcept
{
    VkDescriptorImageInfo imgInfo = {};
    imgInfo.imageView   = imageView;
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write = {};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = dstSet;
    write.dstBinding      = binding;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.pImageInfo      = &imgInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                  PIPELINE LAYOUTS
//------------------------------------------------------------------------------------------------------------------------

VkPipelineLayout constructGBufferPipelineLayout(
    VkDevice device, const FilamentDescriptorLayouts& layouts) noexcept(false)
{
    // ⚙️ GBuffer pipeline uses Set 0 (scene) + Set 1 (material) + push constants (model matrix)
    VkDescriptorSetLayout setLayouts[] = { layouts.sceneLayout, layouts.materialLayout };

    VkPushConstantRange pushRange = {};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset     = 0;
    pushRange.size       = sizeof(ModelPushConstant);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount         = 2;
    layoutInfo.pSetLayouts            = setLayouts;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges    = &pushRange;

    VkPipelineLayout pipelineLayout;
    FILAMENT_VK_VERIFY(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout));
    return pipelineLayout;
}

VkPipelineLayout constructComputePipelineLayout(
    VkDevice device, const FilamentDescriptorLayouts& layouts) noexcept(false)
{
    // ⚙️ Compute pipeline uses single compute layout set
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts    = &layouts.computeLayout;

    VkPipelineLayout pipelineLayout;
    FILAMENT_VK_VERIFY(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout));
    return pipelineLayout;
}

} // namespace VkDescriptorBank
