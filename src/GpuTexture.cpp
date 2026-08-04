#include "GpuTexture.h"

#include "Device.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace VkRenderer
{
namespace
{

VkFormat textureFormat(const TextureAsset& asset)
{
    if (asset.format() != TextureFormat::RGBA8UNorm)
    {
        throw std::invalid_argument(
            "GpuTexture currently requires an RGBA8 TextureAsset");
    }
    return asset.colorSpace() == TextureColorSpace::Srgb
        ? VK_FORMAT_R8G8B8A8_SRGB
        : VK_FORMAT_R8G8B8A8_UNORM;
}

VkFilter textureFilter(TextureFilter filter)
{
    return filter == TextureFilter::Nearest
        ? VK_FILTER_NEAREST
        : VK_FILTER_LINEAR;
}

VkSamplerMipmapMode mipFilter(TextureFilter filter)
{
    return filter == TextureFilter::Nearest
        ? VK_SAMPLER_MIPMAP_MODE_NEAREST
        : VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

VkSamplerAddressMode addressMode(TextureAddressMode mode)
{
    switch (mode)
    {
    case TextureAddressMode::Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case TextureAddressMode::MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case TextureAddressMode::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

} // namespace

GpuTexture::GpuTexture(
    const Device& device,
    UploadContext& uploadContext,
    const TextureAsset& asset)
{
    create(device, uploadContext, asset);
}

GpuTexture::~GpuTexture()
{
    reset();
}

GpuTexture::GpuTexture(GpuTexture&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      image_(std::move(other.image_)),
      view_(std::move(other.view_)),
      sampler_(std::exchange(other.sampler_, VK_NULL_HANDLE))
{
}

GpuTexture& GpuTexture::operator=(GpuTexture&& other) noexcept
{
    if (this != &other)
    {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        image_ = std::move(other.image_);
        view_ = std::move(other.view_);
        sampler_ = std::exchange(other.sampler_, VK_NULL_HANDLE);
    }
    return *this;
}

void GpuTexture::create(
    const Device& device,
    UploadContext& uploadContext,
    const TextureAsset& asset)
{
    if (!device || !asset || asset.mipLevels().empty())
    {
        throw std::invalid_argument(
            "cannot create GpuTexture from incomplete inputs");
    }

    const TextureMipLevel& mip = asset.mipLevels().front();
    if (mip.width != asset.width() || mip.height != asset.height())
    {
        throw std::invalid_argument(
            "GpuTexture requires the first mip to match the base dimensions");
    }

    const VkFormat format = textureFormat(asset);
    GpuTexture replacement;
    replacement.device_ = device.get();
    replacement.image_ = uploadContext.uploadImage2D(
        asset.pixels().data() + mip.byteOffset,
        mip.byteSize,
        mip.width,
        mip.height,
        format);
    replacement.view_.create(
        device.get(),
        replacement.image_.get(),
        format,
        VK_IMAGE_ASPECT_COLOR_BIT);

    const TextureSamplerDesc& sourceSampler = asset.sampler();
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = textureFilter(sourceSampler.magFilter);
    samplerInfo.minFilter = textureFilter(sourceSampler.minFilter);
    samplerInfo.mipmapMode = mipFilter(sourceSampler.mipFilter);
    samplerInfo.addressModeU = addressMode(sourceSampler.addressU);
    samplerInfo.addressModeV = addressMode(sourceSampler.addressV);
    samplerInfo.addressModeW = addressMode(sourceSampler.addressW);
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(
            device.get(),
            &samplerInfo,
            nullptr,
            &replacement.sampler_) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create a texture sampler");
    }

    *this = std::move(replacement);
}

void GpuTexture::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE && sampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(device_, sampler_, nullptr);
    }
    sampler_ = VK_NULL_HANDLE;
    view_.reset();
    image_.reset();
    device_ = VK_NULL_HANDLE;
}

} // namespace VkRenderer
