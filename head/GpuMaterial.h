#pragma once

#include "Asset/MaterialAsset.h"
#include "Buffer.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace VkRenderer
{

class Device;
class GpuTexture;

/// Parameter buffer and texture descriptors compiled for one MaterialAsset.
class GpuMaterial final
{
public:
    GpuMaterial() = default;

    GpuMaterial(const GpuMaterial&) = delete;
    GpuMaterial& operator=(const GpuMaterial&) = delete;
    GpuMaterial(GpuMaterial&&) noexcept = default;
    GpuMaterial& operator=(GpuMaterial&&) noexcept = default;

    void create(
        const Device& device,
        const MaterialAsset& asset,
        const std::vector<const GpuTexture*>& textures,
        VkDescriptorSet descriptorSet);
    void reset() noexcept;

    [[nodiscard]] VkDescriptorSet descriptorSet() const noexcept
    {
        return descriptorSet_;
    }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return parameterBuffer_ && descriptorSet_ != VK_NULL_HANDLE;
    }

private:
    Buffer parameterBuffer_;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
