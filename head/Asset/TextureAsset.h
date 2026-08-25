#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace VkRenderer
{

enum class TextureFormat
{
    Undefined,
    R8UNorm,
    RG8UNorm,
    RGBA8UNorm,
    RGBA16Float,
    RGBA32Float
};

enum class TextureColorSpace
{
    Linear,
    Srgb
};

enum class TextureFilter
{
    Nearest,
    Linear
};

enum class TextureAddressMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge
};

struct TextureSamplerDesc
{
    TextureFilter minFilter = TextureFilter::Linear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureFilter mipFilter = TextureFilter::Linear;
    TextureAddressMode addressU = TextureAddressMode::Repeat;
    TextureAddressMode addressV = TextureAddressMode::Repeat;
    TextureAddressMode addressW = TextureAddressMode::Repeat;
    float maxAnisotropy = 1.0f;
};

struct TextureMipLevel
{
    uint32_t width = 0;
    uint32_t height = 0;
    std::size_t byteOffset = 0;
    std::size_t byteSize = 0;
};

/// Source-independent, CPU-owned texture data ready for GPU upload.
class TextureAsset final
{
public:
    struct CreateInfo
    {
        std::string name;
        uint32_t width = 0;
        uint32_t height = 0;
        TextureFormat format = TextureFormat::Undefined;
        TextureColorSpace colorSpace = TextureColorSpace::Linear;
        TextureSamplerDesc sampler;
        std::vector<std::byte> pixels;
        std::vector<TextureMipLevel> mipLevels;
    };

    TextureAsset() = default;
    explicit TextureAsset(CreateInfo createInfo);

    void create(CreateInfo createInfo);
    void reset() noexcept;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    [[nodiscard]] TextureFormat format() const noexcept { return format_; }
    [[nodiscard]] TextureColorSpace colorSpace() const noexcept { return colorSpace_; }
    [[nodiscard]] const TextureSamplerDesc& sampler() const noexcept { return sampler_; }
    [[nodiscard]] const std::vector<std::byte>& pixels() const noexcept { return pixels_; }
    [[nodiscard]] const std::vector<TextureMipLevel>& mipLevels() const noexcept { return mipLevels_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return width_ != 0 && height_ != 0 &&
            format_ != TextureFormat::Undefined && !pixels_.empty();
    }

private:
    std::string name_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    TextureFormat format_ = TextureFormat::Undefined;
    TextureColorSpace colorSpace_ = TextureColorSpace::Linear;
    TextureSamplerDesc sampler_;
    std::vector<std::byte> pixels_;
    std::vector<TextureMipLevel> mipLevels_;
};

} // namespace VkRenderer
