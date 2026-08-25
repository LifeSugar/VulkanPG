#include "Vulkan/GpuMaterial.h"

#include "Asset/MaterialTemplateAsset.h"
#include "Vulkan/Device.h"
#include "Vulkan/GpuTexture.h"

#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

namespace VkRenderer
{

void GpuMaterial::create(
    const Device& device,
    const MaterialAsset& asset,
    const MaterialTemplateAsset& materialTemplate,
    const std::vector<const GpuTexture*>& textures,
    VkDescriptorSet descriptorSet)
{
    if (!device || !asset || asset.parameterData().empty() ||
        textures.empty() || descriptorSet == VK_NULL_HANDLE)
    {
        throw std::invalid_argument(
            "cannot create GpuMaterial from incomplete inputs");
    }
    for (const GpuTexture* texture : textures)
    {
        if (texture == nullptr || !*texture)
        {
            throw std::invalid_argument(
                "GpuMaterial references an invalid GpuTexture");
        }
    }

    Buffer parameterBuffer(
        device.physical(),
        device.get(),
        asset.parameterData().size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* destination = parameterBuffer.map();
    std::memcpy(
        destination,
        asset.parameterData().data(),
        asset.parameterData().size());
    parameterBuffer.unmap();

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = parameterBuffer.get();
    bufferInfo.range = asset.parameterData().size();

    const std::vector<MaterialTextureSlotDesc>& textureSlots =
        materialTemplate.textureSlots();
    std::vector<VkDescriptorImageInfo> imageInfos(textureSlots.size());
    std::vector<VkDescriptorImageInfo> samplerInfos(textureSlots.size());
    std::vector<VkWriteDescriptorSet> writes(1 + textureSlots.size() * 2);

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding =
        materialTemplate.parameterBlock().descriptor.binding;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &bufferInfo;

    const uint32_t textureSlotCount =
        static_cast<uint32_t>(textureSlots.size());
    for (uint32_t index = 0; index < textureSlotCount; ++index)
    {
        const MaterialTextureSlotDesc& slot = textureSlots[index];
        if (slot.slot >= textures.size())
        {
            throw std::invalid_argument(
                "GpuMaterial texture slot is outside its compiled texture table");
        }
        imageInfos[index].imageView = textures[slot.slot]->view();
        imageInfos[index].imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkWriteDescriptorSet& imageWrite = writes[1 + index];
        imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageWrite.dstSet = descriptorSet;
        imageWrite.dstBinding = slot.imageBinding.binding;
        imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        imageWrite.descriptorCount = 1;
        imageWrite.pImageInfo = &imageInfos[index];

        samplerInfos[index].sampler = textures[slot.slot]->sampler();
        VkWriteDescriptorSet& samplerWrite =
            writes[1 + textureSlotCount + index];
        samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerWrite.dstSet = descriptorSet;
        samplerWrite.dstBinding = slot.samplerBinding.binding;
        samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerWrite.descriptorCount = 1;
        samplerWrite.pImageInfo = &samplerInfos[index];
    }

    vkUpdateDescriptorSets(
        device.get(),
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr);

    reset();
    parameterBuffer_ = std::move(parameterBuffer);
    materialTemplate_ = asset.materialTemplate();
    renderState_ = asset.renderState();
    descriptorSet_ = descriptorSet;
}

void GpuMaterial::reset() noexcept
{
    descriptorSet_ = VK_NULL_HANDLE;
    materialTemplate_ = {};
    renderState_ = {};
    parameterBuffer_.reset();
}

} // namespace VkRenderer
