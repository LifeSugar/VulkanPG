#pragma once

#include "Editor/EditorTexturePreview.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace VkRenderer
{

class RenderAssetCache;

/// Registers cached Vulkan textures with the Dear ImGui Vulkan backend.
class VulkanEditorTexturePreview final
    : public EditorTexturePreviewProvider
{
public:
    void attach(const RenderAssetCache& renderAssets);
    void detach() noexcept;

    [[nodiscard]] EditorTexturePreview preview(
        TextureAssetHandle texture) override;

private:
    struct Entry
    {
        TextureAssetHandle texture;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
    };

    const RenderAssetCache* renderAssets_ = nullptr;
    std::vector<Entry> entries_;
};

} // namespace VkRenderer
