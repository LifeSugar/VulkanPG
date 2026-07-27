#include "FrameDataResources.h"

#include "Device.h"

#include <array>
#include <stdexcept>
#include <vector>

namespace VkRenderer
{

FrameDataResources::FrameDataResources(
    const Device& device,
    uint32_t frameCount,
    uint32_t objectCapacity)
{
    create(device, frameCount, objectCapacity);
}

FrameDataResources::~FrameDataResources()
{
    reset();
}

void FrameDataResources::create(
    const Device& device,
    uint32_t frameCount,
    uint32_t objectCapacity)
{
    if (!device || frameCount == 0 || objectCapacity == 0)
    {
        throw std::invalid_argument(
            "cannot create FrameDataResources with an invalid device or empty capacity");
    }

    reset();

    try
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings(2);
        bindings[0].binding = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT |
            VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding = 2;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        descriptorSetLayout_.create(device.get(), bindings);

        cameraBuffers_.create(
            device,
            frameCount,
            sizeof(CameraGpuData),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        const VkDeviceSize objectBufferSize =
            sizeof(ObjectGpuData) * static_cast<VkDeviceSize>(objectCapacity);
        objectBuffers_.create(
            device,
            frameCount,
            objectBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        const std::vector<VkDescriptorPoolSize> poolSizes = {
            {
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                frameCount * 2
            }
        };
        descriptorPool_.create(device.get(), poolSizes, frameCount);

        descriptorSets_ =
            descriptorPool_.allocate(descriptorSetLayout_.get(), frameCount);

        for (uint32_t i = 0; i < frameCount; ++i)
        {
            VkDescriptorBufferInfo cameraBufferInfo{};
            cameraBufferInfo.buffer = cameraBuffers_.get(i);
            cameraBufferInfo.offset = 0;
            cameraBufferInfo.range = sizeof(CameraGpuData);

            VkDescriptorBufferInfo objectBufferInfo{};
            objectBufferInfo.buffer = objectBuffers_.get(i);
            objectBufferInfo.offset = 0;
            objectBufferInfo.range = objectBufferSize;

            std::array<VkWriteDescriptorSet, 2> writes{};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = descriptorSets_[i];
            writes[0].dstBinding = 1;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[0].descriptorCount = 1;
            writes[0].pBufferInfo = &cameraBufferInfo;

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = descriptorSets_[i];
            writes[1].dstBinding = 2;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &objectBufferInfo;

            vkUpdateDescriptorSets(
                device.get(),
                static_cast<uint32_t>(writes.size()),
                writes.data(),
                0,
                nullptr);
        }
        objectCapacity_ = objectCapacity;
    }
    catch (...)
    {
        reset();
        throw;
    }
}

void FrameDataResources::reset() noexcept
{
    descriptorPool_.reset();
    descriptorSets_.clear();
    objectBuffers_.reset();
    cameraBuffers_.reset();
    descriptorSetLayout_.reset();
    objectCapacity_ = 0;
}

void FrameDataResources::setCameraData(const CameraGpuData& cameraData)
{
    cameraBuffers_.setData(&cameraData, sizeof(cameraData));
}

void FrameDataResources::setObjectData(
    const ObjectGpuData* objectData,
    uint32_t objectCount)
{
    if (objectData == nullptr || objectCount == 0 ||
        objectCount > objectCapacity_)
    {
        throw std::invalid_argument("object data exceeds the frame buffer capacity");
    }
    objectBuffers_.setData(
        objectData,
        sizeof(ObjectGpuData) * static_cast<VkDeviceSize>(objectCount));
}

void FrameDataResources::sync(uint32_t frameIndex)
{
    if (cameraBuffers_.hasStagedData())
    {
        cameraBuffers_.sync(frameIndex);
    }
    if (objectBuffers_.hasStagedData())
    {
        objectBuffers_.sync(frameIndex);
    }
}

VkDescriptorSet FrameDataResources::descriptorSet(uint32_t frameIndex) const
{
    if (frameIndex >= descriptorSets_.size())
    {
        throw std::out_of_range("frame descriptor set index is out of range");
    }
    return descriptorSets_[frameIndex];
}

} // namespace VkRenderer
