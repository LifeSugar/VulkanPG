#pragma once

#include "asset/MaterialAsset.hpp"
#include "vulkan/Buffer.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace rubia::asset
{
class MaterialTemplateAsset;
}

namespace rubia::rhi::vulkan
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
        const asset::MaterialAsset& asset,
        const asset::MaterialTemplateAsset& materialTemplate,
        const std::vector<const GpuTexture*>& textures,
        VkDescriptorSet descriptorSet);
    /// Rewrites only the sampled-image and sampler bindings. Parameter data,
    /// material identity, and the descriptor-set handle stay unchanged.
    void updateTextures(
        const Device& device,
        const asset::MaterialTemplateAsset& materialTemplate,
        const std::vector<const GpuTexture*>& textures);
    void reset() noexcept;

    [[nodiscard]] VkDescriptorSet descriptorSet() const noexcept
    {
        return descriptorSet_;
    }
    [[nodiscard]] const asset::MaterialRenderState& renderState() const noexcept
    {
        return renderState_;
    }
    [[nodiscard]] asset::MaterialTemplateAssetHandle materialTemplate() const
        noexcept
    {
        return materialTemplate_;
    }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return parameterBuffer_ && materialTemplate_ &&
            descriptorSet_ != VK_NULL_HANDLE;
    }

private:
    Buffer parameterBuffer_;
    asset::MaterialTemplateAssetHandle materialTemplate_;
    asset::MaterialRenderState renderState_;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
};

} // namespace rubia::rhi::vulkan
