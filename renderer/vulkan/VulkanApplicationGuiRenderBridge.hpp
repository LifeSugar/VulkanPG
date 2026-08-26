#pragma once

#include "render/ApplicationGuiRenderBridge.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace VkRenderer
{

class RenderAssetCache;
class VulkanRenderer;

/// Adapts Vulkan render outputs and cached textures to opaque GUI tokens.
class VulkanApplicationGuiRenderBridge final
    : public ApplicationGuiRenderBridge
{
public:
    ~VulkanApplicationGuiRenderBridge() override;

    void attach(
        VulkanRenderer& renderer,
        const RenderAssetCache& renderAssets);
    void detach() noexcept;

    void resizeSceneViewport(uint32_t width, uint32_t height) override;
    [[nodiscard]] ApplicationGuiRenderFrame currentFrame() override;
    [[nodiscard]] ApplicationGuiTexture preview(
        TextureAssetHandle texture) override;
    void invalidatePreview(TextureAssetHandle texture) noexcept override;

private:
    struct TextureEntry
    {
        TextureAssetHandle texture;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
    };

    void registerViewportTextures();
    void refreshViewportTexturesIfNeeded();
    void releaseViewportTextures() noexcept;
    void releasePreviewTextures() noexcept;

    VulkanRenderer* renderer_ = nullptr;
    const RenderAssetCache* renderAssets_ = nullptr;
    std::vector<VkDescriptorSet> viewportTextures_;
    std::vector<TextureEntry> previewTextures_;
    uint64_t viewportTextureRevision_ = 0;
};

} // namespace VkRenderer
