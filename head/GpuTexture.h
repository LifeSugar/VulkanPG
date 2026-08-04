#pragma once

#include "Asset/TextureAsset.h"
#include "Image.h"
#include "ImageView.h"
#include "UploadContext.h"

#include <vulkan/vulkan.h>

namespace VkRenderer
{

/// Vulkan sampling resources created from one source-independent TextureAsset.
class GpuTexture final
{
public:
    GpuTexture() = default;
    GpuTexture(
        const Device& device,
        UploadContext& uploadContext,
        const TextureAsset& asset);
    ~GpuTexture();

    GpuTexture(const GpuTexture&) = delete;
    GpuTexture& operator=(const GpuTexture&) = delete;
    GpuTexture(GpuTexture&& other) noexcept;
    GpuTexture& operator=(GpuTexture&& other) noexcept;

    void create(
        const Device& device,
        UploadContext& uploadContext,
        const TextureAsset& asset);
    void reset() noexcept;

    [[nodiscard]] VkImageView view() const noexcept { return view_.get(); }
    [[nodiscard]] VkSampler sampler() const noexcept { return sampler_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return image_ && view_ && sampler_ != VK_NULL_HANDLE;
    }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    Image image_;
    ImageView view_;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

} // namespace VkRenderer
