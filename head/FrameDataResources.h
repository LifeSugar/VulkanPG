#pragma once

#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "PerFrameBuffer.h"
#include "RenderData.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace VkRenderer
{

class Device;

class FrameDataResources final
{
public:
    FrameDataResources() = default;
    FrameDataResources(
        const Device& device,
        uint32_t frameCount,
        uint32_t objectCapacity = 1);
    ~FrameDataResources();

    FrameDataResources(const FrameDataResources&) = delete;
    FrameDataResources& operator=(const FrameDataResources&) = delete;
    FrameDataResources(FrameDataResources&&) = delete;
    FrameDataResources& operator=(FrameDataResources&&) = delete;

    void create(
        const Device& device,
        uint32_t frameCount,
        uint32_t objectCapacity = 1);
    void reset() noexcept;

    void setCameraData(const CameraGpuData& cameraData);
    void setObjectData(const ObjectGpuData* objectData, uint32_t objectCount);
    void sync(uint32_t frameIndex);

    [[nodiscard]] VkDescriptorSetLayout descriptorSetLayout() const noexcept
    {
        return descriptorSetLayout_.get();
    }
    [[nodiscard]] VkDescriptorSet descriptorSet(uint32_t frameIndex) const;
    [[nodiscard]] uint32_t frameCount() const noexcept
    {
        return static_cast<uint32_t>(descriptorSets_.size());
    }
    [[nodiscard]] uint32_t objectCapacity() const noexcept
    {
        return objectCapacity_;
    }

private:
    // Declaration order keeps the implicit destruction order safe:
    // pool -> descriptor handles -> buffers -> layout.
    DescriptorSetLayout descriptorSetLayout_;
    PerFrameBuffer cameraBuffers_;
    PerFrameBuffer objectBuffers_;
    std::vector<VkDescriptorSet> descriptorSets_;
    DescriptorPool descriptorPool_;
    uint32_t objectCapacity_ = 0;
};

} // namespace VkRenderer
